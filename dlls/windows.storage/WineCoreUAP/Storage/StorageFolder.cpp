/* WinRT Windows.Storage.StorageFolder Implementation.
 *
 * Written by Weather
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <atomic>
#include <mutex>
#include <cwchar>

#include "../../private.h"
#include "provider.h"

#include <initguid.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shobjidl.h>
#include <pathcch.h>

#include "../Foundation/IWineAsync.hpp"

WINE_DEFAULT_DEBUG_CHANNEL(storage);

using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Storage;
using namespace ABI::Windows::Storage::FileProperties;

// FIXME: Figure out name collision 
#undef CreateFile

static VOID GenerateUniqueFolderName( LPWSTR buffer, SIZE_T bufferSize )
{
    UUID uuid;
    LPWSTR str;

    UuidCreate( &uuid );
    UuidToStringW( &uuid, (RPC_WSTR*)&str );
    swprintf( (wchar_t *)buffer, bufferSize, L"%s", str );

    RpcStringFreeW( (RPC_WSTR*)&str );
}

static INT64 FileTimeToUnixTime( const FILETIME *ft )
{
    ULARGE_INTEGER ull;

    ull.LowPart = ft->dwLowDateTime;
    ull.HighPart = ft->dwHighDateTime;

    return ( ull.QuadPart / WINDOWS_TICK ) - SEC_TO_UNIX_EPOCH;
}

class ABI::Windows::Storage::StorageFolder final
    : public IStorageFolder
    , public IStorageItem
{
public:
    StorageFolder() = default;

    StorageFolder( IShellItem *item )
    {
        item->AddRef();
        this->item = item;
        this->canBeModified = TRUE;
    }

    StorageFolder( IShellItem *item, BOOL canBeModified )
    {
        item->AddRef();
        this->item = item;
        this->canBeModified = canBeModified;
    }

    /* IUnknown Methods (Shared) */
    HRESULT WINAPI
    QueryInterface( REFIID iid, void** out ) noexcept override
    {
        TRACE( "iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );

        if (!out) return E_POINTER;
        *out = nullptr;

        if ( iid == __uuidof( IUnknown ) ||
             iid == __uuidof( IInspectable ) ||
             iid == __uuidof( IAgileObject ) ||
             iid == __uuidof( IStorageFolder ) )
        {
            AddRef();
            *out = static_cast<IStorageFolder *>(this);
            return S_OK;
        }

        if ( iid == __uuidof( IStorageItem ) )
        {
            AddRef();
            *out = static_cast<IStorageItem *>(this);
            return S_OK;
        }

        FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( &iid ) );
        *out = nullptr;
        return E_NOINTERFACE;
    }

    ULONG WINAPI 
    AddRef() noexcept override
    {
        ULONG curr = static_cast<ULONG>(++ref);
        TRACE( "iface %p increasing refcount to %lu.\n", this, curr );
        return curr;
    }

    ULONG WINAPI 
    Release() noexcept override
    {
        ULONG curr = static_cast<ULONG>(--ref);
        TRACE( "iface %p decreasing refcount to %lu.\n", this, curr );

        if ( !curr )
            delete this;

        return curr;
    }

    /* IInspectable Methods (Shared) */
    HRESULT WINAPI
    GetIids( ULONG *iid_count, IID **iids ) noexcept override
    {
        TRACE( "iface %p, iid_count %p, iids %p\n", this, iid_count, iids );

        if ( !iid_count || !iids )
            return E_POINTER;
        
        *iid_count = 2;
        IID* allocated = static_cast<IID*>( CoTaskMemAlloc( sizeof(IID) * (*iid_count) ) );

        if ( !allocated )
            return E_OUTOFMEMORY;

        allocated[0] = __uuidof( IActivationFactory );
        allocated[1] = __uuidof( IStorageFolderStatics );

        *iids = allocated;
        return S_OK;
    }

    HRESULT WINAPI
    GetRuntimeClassName( HSTRING *class_name ) noexcept override
    {
        TRACE( "iface %p, class_name %p\n", this, class_name );
        return WindowsCreateString( (LPCWSTR)L"Windows.Storage.StorageFolder", 30, class_name );
    }

    HRESULT WINAPI
    GetTrustLevel( TrustLevel *trust_level ) noexcept override
    {
        FIXME( "iface %p, trust_level %p stub!\n", this, trust_level );
        return E_NOTIMPL;
    }

    /* IStorageItem Methods */
    HRESULT WINAPI
    RenameAsyncOverloadDefaultOptions( HSTRING name, IAsyncAction **operation ) noexcept override
    {
        TRACE( "iface %p, name %s, operation %p.\n", this, debugstr_hstring( name ), operation );
        return RenameAsync( name, NameCollisionOption::FailIfExists, operation );
    }

    HRESULT WINAPI
    RenameAsync( HSTRING name, NameCollisionOption option, IAsyncAction **operation ) noexcept override
    {
        HRESULT hr;
        StorageItem_RenameOptions *options;

        TRACE( "iface %p, name %s, option %d, operation %p.\n", this, debugstr_hstring( name ), (INT)option, operation );

        if ( !name || WindowsIsStringEmpty( name ) ) 
            return E_INVALIDARG;
        if ( !operation ) 
            return E_POINTER;

        options = new StorageItem_RenameOptions();
        options->name = name;
        options->option = option;

        hr = AsyncAction::Create( reinterpret_cast<IUnknown *>(this), 
            static_cast<PVOID>(options), Rename, operation );
        TRACE( "created IAsyncAction %p.\n", *operation );

        return hr;
    }

    HRESULT WINAPI
    DeleteAsyncOverloadDefaultOptions( IAsyncAction **operation ) noexcept override
    {
        TRACE( "iface %p, operation %p.\n", this, operation );
        return DeleteAsync( StorageDeleteOption::Default, operation );
    }

    HRESULT WINAPI
    DeleteAsync( StorageDeleteOption option, IAsyncAction **operation ) noexcept override
    {
        HRESULT hr;
        StorageItem_DeleteOptions *options;

        TRACE( "iface %p, option %d, operation %p.\n", this, (INT)option, operation );

        if ( !operation ) 
            return E_POINTER;

        options = new StorageItem_DeleteOptions();
        options->option = option;

        hr = AsyncAction::Create( reinterpret_cast<IUnknown *>(this), 
            static_cast<PVOID>(options), Delete, operation );
        TRACE( "created IAsyncAction %p.\n", *operation );

        return hr;
    }

    HRESULT WINAPI
    GetBasicPropertiesAsync( IAsyncOperation<BasicProperties *> **operation ) noexcept override
    {
        FIXME( "iface %p, operation %p stub!\n", this, operation );

        return E_NOTIMPL;
    }

    HRESULT WINAPI
    get_Name( HSTRING *out ) noexcept override
    {
        LPWSTR name;
        HRESULT hr;

        TRACE( "iface %p, out %p.\n", this, out );

        if ( !out )
            return E_POINTER;
        
        hr = this->item->GetDisplayName( SIGDN_NORMALDISPLAY, &name );
        if ( FAILED( hr ) ) return hr;

        hr = WindowsCreateString( name, lstrlenW( name ), out );

        CoTaskMemFree( name );
        return hr;
    }

    HRESULT WINAPI
    get_Path( HSTRING *out ) noexcept override
    {
        LPWSTR path;
        HRESULT hr;

        TRACE( "iface %p, out %p.\n", this, out );

        if ( !out )
            return E_POINTER;
        
        hr = this->item->GetDisplayName( SIGDN_FILESYSPATH, &path );
        if ( FAILED( hr ) ) return hr;

        hr = WindowsCreateString( path, lstrlenW( path ), out );

        CoTaskMemFree( path );
        return hr;
    }

    HRESULT WINAPI
    get_Attributes( FileAttributes *attributes )
    {
        HSTRING itemPath;
        HRESULT hr;
        DWORD attribs;

        TRACE( "iface %p, attributes %p.\n", this, attributes );

        if ( !attributes )
            return E_POINTER;

        hr = get_Path( &itemPath );
        if ( FAILED( hr ) ) return hr;

        // FIXME: IShellItem2::GetUINT32 is not implemented!
        attribs = GetFileAttributesW( WindowsGetStringRawBuffer( itemPath, nullptr ) );

        if ( attribs & FILE_ATTRIBUTE_READONLY )
            *attributes |= FileAttributes::ReadOnly;

        if ( attribs & FILE_ATTRIBUTE_DIRECTORY )
            *attributes |= FileAttributes::Directory;

        if ( attribs & FILE_ATTRIBUTE_ARCHIVE )
            *attributes |= FileAttributes::Archive;

        if ( attribs & FILE_ATTRIBUTE_TEMPORARY )
            *attributes |= FileAttributes::Temporary;

        if ( itemPath ) WindowsDeleteString( itemPath );
        return hr;
    }

    HRESULT WINAPI
    get_DateCreated( DateTime *created ) noexcept override
    {
        FILETIME itemCreatedTime;
        HRESULT hr;
        HSTRING itemPath = NULL;
        HANDLE itemHandle;

        TRACE( "iface %p, created %p.\n", this, created );

        hr = get_Path( &itemPath );
        if ( FAILED( hr ) ) goto _CLEANUP;

        itemHandle = CreateFileW( WindowsGetStringRawBuffer( itemPath, nullptr ), GENERIC_READ, FILE_SHARE_READ, nullptr,
                       OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr );
        if ( itemHandle == INVALID_HANDLE_VALUE )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto _CLEANUP;
        }

        if ( !GetFileTime( itemHandle, &itemCreatedTime, nullptr, nullptr ) ) 
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto _CLEANUP;
        }

        created->UniversalTime = FileTimeToUnixTime( &itemCreatedTime );

_CLEANUP:
        if ( itemPath ) WindowsDeleteString( itemPath );
        if ( itemHandle && itemHandle != INVALID_HANDLE_VALUE ) CloseHandle( itemHandle );
        return hr;
    }

    HRESULT WINAPI
    IsOfType( StorageItemTypes type, boolean *value ) noexcept override
    {
        TRACE( "iface %p, type %d, value %p.\n", this, (INT)type, value );
        *value = type == StorageItemTypes::Folder;
        return S_OK;
    }

    /* IStorageFolder Methods */
    HRESULT WINAPI
    CreateFileAsyncOverloadDefaultOptions( HSTRING name, IAsyncOperation<StorageFile *> **operation ) noexcept override
    {
        TRACE( "iface %p, name %s, operation %p", this, debugstr_hstring(name), operation );
        return CreateFileAsync( name, CreationCollisionOption::FailIfExists, operation );
    }
    
    HRESULT WINAPI
    CreateFileAsync( HSTRING name, CreationCollisionOption option, IAsyncOperation<StorageFile *> **operation ) noexcept override
    {
        FIXME( "iface %p, name %s, option %d, operation %p stub!\n", this, debugstr_hstring(name), (INT)option, operation );
        return E_NOTIMPL;
    }

    HRESULT WINAPI
    CreateFolderAsyncOverloadDefaultOptions( HSTRING name, IAsyncOperation<StorageFolder *> **operation ) noexcept override
    {
        TRACE( "iface %p, name %s, operation %p", this, debugstr_hstring(name), operation );
        return CreateFolderAsync( name, CreationCollisionOption::FailIfExists, operation );
    }

    HRESULT WINAPI
    CreateFolderAsync( HSTRING name, CreationCollisionOption option, IAsyncOperation<StorageFolder *> **operation ) noexcept override
    {
        FIXME( "iface %p, name %s, option %d, operation %p stub!\n", this, debugstr_hstring(name), (INT)option, operation );
        return E_NOTIMPL;
    }

    HRESULT WINAPI
    GetFileAsync( HSTRING name, IAsyncOperation<StorageFile *> **operation ) noexcept override
    {
        FIXME( "iface %p, name %p, operation %p stub!\n", this, name, operation );
        return E_NOTIMPL;
    }

    HRESULT WINAPI
    GetFolderAsync( HSTRING name, IAsyncOperation<StorageFolder *> **operation ) noexcept override
    {
        FIXME( "iface %p, name %p, operation %p stub!\n", this, name, operation );
        return E_NOTIMPL;
    }

    HRESULT WINAPI
    GetItemAsync( HSTRING name, IAsyncOperation<IStorageItem *> **operation ) noexcept override
    {
        FIXME( "iface %p, name %p, operation %p stub!\n", this, name, operation );
        return E_NOTIMPL;
    }

    HRESULT WINAPI
    GetFilesAsyncOverloadDefaultOptionsStartAndCount( IAsyncOperation<IVectorView<StorageFile *> *> **operation ) noexcept override
    {
        FIXME( "iface %p, operation %p stub!\n", this, operation );
        return E_NOTIMPL;
    }

    HRESULT WINAPI
    GetFoldersAsyncOverloadDefaultOptionsStartAndCount( IAsyncOperation<IVectorView<StorageFolder *> *> **operation ) noexcept override
    {
        FIXME( "iface %p, operation %p stub!\n", this, operation );
        return E_NOTIMPL;
    }

    HRESULT WINAPI
    GetItemsAsyncOverloadDefaultStartAndCount( IAsyncOperation<IVectorView<IStorageItem *> *> **operation ) noexcept override
    {
        FIXME( "iface %p, operation %p stub!\n", this, operation );
        return E_NOTIMPL;
    }

protected:
    struct StorageItem_RenameOptions
    {
        HSTRING name;
        NameCollisionOption option;
    };

    struct StorageItem_DeleteOptions
    {
        StorageDeleteOption option;
    };

private:
    static HRESULT WINAPI
    Rename( IUnknown *invoker, PVOID param, PROPVARIANT *result )
    {
        WCHAR newItemPath[MAX_PATH];
        WCHAR folderRename[MAX_PATH];
        HRESULT hr;
        HSTRING oldPath;
        StorageFolder *impl = reinterpret_cast<StorageFolder *>( invoker );
        IFileOperation *fileOperation;

        auto *options = static_cast<StorageItem_RenameOptions *>( param );

        TRACE( "invoker %p, param %p, result %p.\n", invoker, param, result );

        if ( !impl->canBeModified )
        {
            hr = E_ACCESSDENIED;
            goto _CLEANUP;
        }
        hr = CoCreateInstance( CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_PPV_ARGS( &fileOperation ) );
        if ( FAILED( hr ) ) goto _CLEANUP;

        fileOperation->SetOperationFlags( FOF_NO_UI );

        impl->get_Path( &oldPath );

        lstrcpyW( newItemPath, WindowsGetStringRawBuffer( oldPath, nullptr ) );
        lstrcpyW( folderRename, WindowsGetStringRawBuffer( options->name, nullptr ) );

        hr = PathCchRemoveFileSpec( newItemPath, MAX_PATH );
        if ( FAILED( hr ) ) goto _CLEANUP;

        hr = PathAppendW( newItemPath, WindowsGetStringRawBuffer( options->name, nullptr ) );
        if ( FAILED( hr ) ) goto _CLEANUP;

        switch( options->option )
        {
            case NameCollisionOption::ReplaceExisting:
            {
                if ( PathFileExistsW( newItemPath ) )
                    if ( !DeleteFileW( newItemPath ) )
                    {
                        hr = HRESULT_FROM_WIN32( GetLastError() );
                        goto _CLEANUP;
                    }
                break;
            }

            case NameCollisionOption::FailIfExists:
            {
                if ( PathFileExistsW( newItemPath ) )
                {
                    hr = HRESULT_FROM_WIN32( ERROR_FILE_EXISTS );
                    goto _CLEANUP;
                }
                break;
            }

            case NameCollisionOption::GenerateUniqueName:
            {
                GenerateUniqueFolderName( folderRename, sizeof(folderRename) );
                break;
            }
        }

        hr = fileOperation->RenameItem( impl->item, folderRename, nullptr );
        if ( FAILED( hr ) ) goto _CLEANUP;

        hr = fileOperation->PerformOperations();
        if ( FAILED( hr ) ) goto _CLEANUP;

_CLEANUP:
        if ( fileOperation ) fileOperation->Release();
        if ( oldPath ) WindowsDeleteString( oldPath );
        delete options;
        return hr;
    }

    static HRESULT WINAPI
    Delete( IUnknown *invoker, PVOID param, PROPVARIANT *result )
    {
        HRESULT hr;
        StorageFolder *impl = reinterpret_cast<StorageFolder *>( invoker );
        IFileOperation *fileOperation;

        auto *options = static_cast<StorageItem_DeleteOptions *>( param );

        TRACE( "invoker %p, param %p, result %p.\n", invoker, param, result );

        if ( !impl->canBeModified )
        {
            hr = E_ACCESSDENIED;
            goto _CLEANUP;
        }
        hr = CoCreateInstance( CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_PPV_ARGS( &fileOperation ) );
        if ( FAILED( hr ) ) goto _CLEANUP;

        fileOperation->SetOperationFlags( FOF_NO_UI );

        switch ( options->option )
        {
            case StorageDeleteOption::Default:
                // FIXME: FOFX_RECYCLEONDELETE is not implemented!
                fileOperation->SetOperationFlags( FOF_NO_UI | FOFX_RECYCLEONDELETE );
            case StorageDeleteOption::PermanentDelete:
                hr = fileOperation->DeleteItem( impl->item, nullptr );
                if ( FAILED( hr ) ) goto _CLEANUP;
                break;
        }

        hr = fileOperation->PerformOperations();
        if ( FAILED( hr ) ) goto _CLEANUP;


_CLEANUP:
        if ( fileOperation ) fileOperation->Release();
        delete options;
        return hr;
    }

    std::atomic_long ref{ 1 };
    
    IShellItem *item;
    BOOLEAN canBeModified;
};


class StorageFolderImpl final
    : public IActivationFactory
    , public IStorageFolderStatics
{
public:
    StorageFolderImpl() = default;

    /* IUnknown Methods (Shared) */
    HRESULT WINAPI
    QueryInterface( REFIID iid, void** out ) noexcept override
    {
        TRACE( "iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );

        if (!out) return E_POINTER;
        *out = nullptr;

        if ( iid == __uuidof( IUnknown ) ||
             iid == __uuidof( IInspectable ) ||
             iid == __uuidof( IAgileObject ) ||
             iid == __uuidof( IActivationFactory ) )
        {
            AddRef();
            *out = static_cast<IActivationFactory *>(this);
            return S_OK;
        }

        if ( iid == __uuidof( IStorageFolderStatics ) )
        {
            AddRef();
            *out = static_cast<IStorageFolderStatics *>(this);
            return S_OK;
        }

        FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( &iid ) );
        *out = nullptr;
        return E_NOINTERFACE;
    }

    ULONG WINAPI 
    AddRef() noexcept override
    {
        ULONG curr = static_cast<ULONG>(++ref);
        TRACE( "iface %p increasing refcount to %lu.\n", this, curr );
        return curr;
    }

    ULONG WINAPI 
    Release() noexcept override
    {
        ULONG curr = static_cast<ULONG>(--ref);
        TRACE( "iface %p decreasing refcount to %lu.\n", this, curr );

        if ( !curr )
            delete this;

        return curr;
    }

    /* IInspectable Methods (Shared) */
    HRESULT WINAPI
    GetIids( ULONG *iid_count, IID **iids ) noexcept override
    {
        TRACE( "iface %p, iid_count %p, iids %p\n", this, iid_count, iids );

        if ( !iid_count || !iids )
            return E_POINTER;
        
        *iid_count = 2;
        IID* allocated = static_cast<IID*>( CoTaskMemAlloc( sizeof(IID) * (*iid_count) ) );

        if ( !allocated )
            return E_OUTOFMEMORY;

        allocated[0] = __uuidof( IActivationFactory );
        allocated[1] = __uuidof( IStorageFolderStatics );

        *iids = allocated;
        return S_OK;
    }

    HRESULT WINAPI
    GetRuntimeClassName( HSTRING *class_name ) noexcept override
    {
        TRACE( "iface %p, class_name %p\n", this, class_name );
        return WindowsCreateString( (LPCWSTR)L"Windows.Storage.StorageFolder", 30, class_name );
    }

    HRESULT WINAPI
    GetTrustLevel( TrustLevel *trust_level ) noexcept override
    {
        FIXME( "iface %p, trust_level %p stub!\n", this, trust_level );
        return E_NOTIMPL;
    }

    /* IActivationFactory Methods */
    HRESULT WINAPI
    ActivateInstance( IInspectable **instance ) noexcept override
    {
        ERR( "iface %p, This factory is not activatable!\n", this );
        return E_NOTIMPL;
    }

    /* IStorageFolderStatics Methods */
    HRESULT WINAPI
    GetFolderFromPathAsync( HSTRING path, IAsyncOperation<StorageFolder *> **result ) noexcept override 
    {
        HRESULT hr;
        StorageFolderImpl_GetFolderFromPathOptions *options;

        TRACE( "iface %p, path %s, operation %p\n", this, debugstr_hstring( path ), result );

        //Arguments
        if ( !path ) return E_INVALIDARG;
        if ( !result ) return E_POINTER;

        options = new StorageFolderImpl_GetFolderFromPathOptions();
        options->path = path;

        hr = AsyncOperation::Inspectable::Create( reinterpret_cast<IUnknown *>(this), 
            static_cast<PVOID>(options), GetFolderFromPath, (IAsyncOperation<IInspectable *> **)result );
        TRACE( "created IAsyncOperation_StorageFolder %p.\n", *result );

        return hr;
    }

protected:
    struct StorageFolderImpl_GetFolderFromPathOptions
    {
        HSTRING path;
    };

private:
    static HRESULT WINAPI
    GetFolderFromPath( IUnknown *invoker, PVOID param, PROPVARIANT *result )
    {
        HRESULT hr;
        IShellItem *folderItem;
        StorageFolder *folder;

        auto *options = static_cast<StorageFolderImpl_GetFolderFromPathOptions *>(param);

        TRACE( "invoker %p, param %p, result %p\n", invoker, param, result );

        hr = SHCreateItemFromParsingName( WindowsGetStringRawBuffer( options->path, nullptr), nullptr, IID_PPV_ARGS(&folderItem) );
        if ( FAILED( hr ) ) goto _CLEANUP;

        folder = new StorageFolder( folderItem );
        
        result->punkVal = reinterpret_cast<IUnknown *>( folder );

_CLEANUP:
        delete options;
        return S_OK;
    }

    std::atomic_long ref{ 1 };
};

static StorageFolderImpl g_storage_folder_statics;

IActivationFactory* storage_folder_factory =
    static_cast<IActivationFactory*>(&g_storage_folder_statics);