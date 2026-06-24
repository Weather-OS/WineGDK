/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XStore
 *
 * Copyright 2026 Olivia Ryan
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

#include "private.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

class XStoreImpl : public IXStoreImpl6
{
public:
    HRESULT WINAPI QueryInterface( REFIID iid, void **out ) override
    {
        TRACE( "iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );

        if (iid == __uuidof( IUnknown     ) ||
            iid == __uuidof( IXStoreImpl  ) ||
            iid == __uuidof( IXStoreImpl2 ) ||
            iid == __uuidof( IXStoreImpl3 ) ||
            iid == __uuidof( IXStoreImpl4 ) ||
            iid == __uuidof( IXStoreImpl5 ) ||
            iid == __uuidof( IXStoreImpl6 ))
        {
            AddRef();
            *out = static_cast<IXStoreImpl6 *>(this);
            return S_OK;
        }

        FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( &iid ) );
        *out = nullptr;
        return E_NOINTERFACE;
    }

    ULONG WINAPI AddRef() override
    {
        ULONG ref = InterlockedIncrement( &this->ref );
        TRACE( "iface %p increasing refcount to %lu.\n", this, ref );
        return ref;
    }

    ULONG WINAPI Release() override
    {
        ULONG ref = InterlockedDecrement( &this->ref );
        TRACE( "iface %p decreasing refcount to %lu.\n", this, ref );
        return ref;
    }

    HRESULT WINAPI XStoreCreateContext( const XUserHandle user, XStoreContextHandle *storeContextHandle ) override
    {
        FIXME( "iface %p, user %p, storeContextHandle %p stub!\n", this, user, storeContextHandle );
        return E_NOTIMPL;
    }

    void WINAPI XStoreCloseContextHandle( XStoreContextHandle storeContextHandle ) override
    {
        FIXME( "iface %p, storeContextHandle %p stub!\n", this, storeContextHandle );
    }

    HRESULT WINAPI XStoreQueryAssociatedProductsAsync( const XStoreContextHandle storeContextHandle, XStoreProductKind productKinds, UINT32 maxItemsToRetrievePerPage, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, productKinds %#x, maxItemsToRetrievePerPage %u, async %p stub!\n", this, storeContextHandle, static_cast<UINT32>(productKinds), maxItemsToRetrievePerPage, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryAssociatedProductsResult( XAsyncBlock *async, XStoreProductQueryHandle *productQueryHandle ) override
    {
        FIXME( "iface %p, async %p, productQueryHandle %p stub!\n", this, async, productQueryHandle );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryProductsAsync( const XStoreContextHandle storeContextHandle, XStoreProductKind productKinds, const char **storeIds, SIZE_T storeIdsCount, const char **actionFilters, SIZE_T actionFiltersCount, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, productKinds %#x, storeIds %p, storeIdsCount %Iu, actionFilters %p, actionFiltersCount %Iu, async %p stub!\n", this, storeContextHandle, static_cast<UINT32>(productKinds), storeIds, storeIdsCount, actionFilters, actionFiltersCount, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryProductsResult( XAsyncBlock *async, XStoreProductQueryHandle *productQueryHandle ) override
    {
        FIXME( "iface %p, async %p, productQueryHandle %p stub!\n", this, async, productQueryHandle );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryEntitledProductsAsync( const XStoreContextHandle storeContextHandle, XStoreProductKind productKinds, UINT32 maxItemsToRetrievePerPage, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, productKinds %#x, maxItemsToRetrievePerPage, %u, async %p stub!\n", this, storeContextHandle, static_cast<UINT32>(productKinds), maxItemsToRetrievePerPage, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryEntitledProductsResult( XAsyncBlock *async, XStoreProductQueryHandle *productQueryHandle ) override
    {
        FIXME( "iface %p, async %p, productQueryHandle %p stub!\n", this, async, productQueryHandle );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryProductForCurrentGameAsync( const XStoreContextHandle storeContextHandle, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, async %p stub!\n", this, storeContextHandle, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryProductForCurrentGameResult( XAsyncBlock *async, XStoreProductQueryHandle *productQueryHandle ) override
    {
        FIXME( "iface %p, async %p, productQueryHandle %p stub!\n", this, async, productQueryHandle );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryProductForPackageAsync( const XStoreContextHandle storeContextHandle, XStoreProductKind productKinds, const char *packageIdentifier, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, productKinds %#x, packageIdentifier %s, async %p stub!\n", this, storeContextHandle, static_cast<UINT32>(productKinds), debugstr_a( packageIdentifier ), async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryProductForPackageResult( XAsyncBlock *async, XStoreProductQueryHandle *productQueryHandle ) override
    {
        FIXME( "iface %p, async %p, productQueryHandle %p stub!\n", this, async, productQueryHandle );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreEnumerateProductsQuery( const XStoreProductQueryHandle productQueryHandle, void *context, XStoreProductQueryCallback *callback ) override
    {
        FIXME( "iface %p, productQueryHandle %p, context %p, callback %p stub!\n", this, productQueryHandle, context, callback );
        return E_NOTIMPL;
    }

    BOOLEAN WINAPI XStoreProductsQueryHasMorePages( const XStoreProductQueryHandle productQueryHandle ) override
    {
        FIXME( "iface %p, productQueryHandle %p stub!\n", this, productQueryHandle );
        return FALSE;
    }

    HRESULT WINAPI XStoreProductsQueryNextPageAsync( const XStoreProductQueryHandle productQueryHandle, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, productQueryHandle %p, async %p stub!\n", this, productQueryHandle, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreProductsQueryNextPageResult( XAsyncBlock *async, XStoreProductQueryHandle *productQueryHandle ) override
    {
        FIXME( "iface %p, async %p, productQueryHandle %p stub!\n", this, async, productQueryHandle );
        return E_NOTIMPL;
    }

    void WINAPI XStoreCloseProductsQueryHandle( XStoreProductQueryHandle productQueryHandle ) override
    {
        FIXME( "iface %p, productQueryHandle %p stub!\n", this, productQueryHandle );
    }

    HRESULT WINAPI XStoreAcquireLicenseForPackageAsync( const XStoreProductQueryHandle productQueryHandle, const char *packageIdentifier, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, productQueryHandle %p, packageIdentifier %s, async %p stub!\n", this, productQueryHandle, debugstr_a( packageIdentifier ), async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreAcquireLicenseForPackageResult( XAsyncBlock *async, XStoreLicenseHandle *storeLicenseHandle ) override
    {
        FIXME( "iface %p, async %p, storeLicenseHandle %p stub!\n", this, async, storeLicenseHandle );
        return E_NOTIMPL;
    }

    BOOLEAN WINAPI XStoreIsLicenseValid( const XStoreLicenseHandle storeLicenseHandle ) override
    {
        FIXME( "iface %p, storeLicenseHandle %p stub!\n", this, storeLicenseHandle );
        return FALSE;
    }

    void WINAPI XStoreCloseLicenseHandle( XStoreLicenseHandle storeLicenseHandle ) override
    {
        FIXME( "iface %p, storeLicenseHandle %p stub!\n", this, storeLicenseHandle );
    }

    HRESULT WINAPI XStoreCanAcquireLicenseForStoreIdAsync( const XStoreContextHandle storeContextHandle, const char *storeProductId, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, storeProductId %s, async %p stub!\n", this, storeContextHandle, debugstr_a( storeProductId ), async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreCanAcquireLicenseForStoreIdResult( XAsyncBlock *async, XStoreCanAcquireLicenseResult *storeCanAcquireLicense ) override
    {
        FIXME( "iface %p, async %p, storeCanAcquireLicense %p stub!\n", this, async, storeCanAcquireLicense );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreCanAcquireLicenseForPackageAsync( const XStoreContextHandle storeContextHandle, const char *packageIdentifier, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, packageIdentifier %s, async %p stub!\n", this, storeContextHandle, debugstr_a( packageIdentifier ), async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreCanAcquireLicenseForPackageResult( XAsyncBlock *async, XStoreCanAcquireLicenseResult *storeCanAcquireLicense ) override
    {
        FIXME( "iface %p, async %p, storeCanAcquireLicense %p stub!\n", this, async, storeCanAcquireLicense );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryGameLicenseAsync( const XStoreContextHandle storeContextHandle, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, async %p stub!\n", this, storeContextHandle, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryGameLicenseResult( XAsyncBlock *async, XStoreGameLicense *license ) override
    {
        FIXME( "iface %p, async %p, license %p stub!\n", this, async, license );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryAddOnLicensesAsync( const XStoreContextHandle storeContextHandle, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, async %p stub!\n", this, storeContextHandle, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryAddOnLicensesResultCount( XAsyncBlock *async, UINT32 *count ) override
    {
        FIXME( "iface %p, async %p, count %p stub!\n", this, async, count );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryAddOnLicensesResult( XAsyncBlock *async, UINT32 count, XStoreAddonLicense *addOnLicenses ) override
    {
        FIXME( "iface %p, async %p, count %u, addOnLicenses %p stub!\n", this, async, count, addOnLicenses );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryConsumableBalanceRemainingAsync( const XStoreContextHandle storeContextHandle, const char *storeProductId, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, storeProductId %s, async %p stub!\n", this, storeContextHandle, debugstr_a( storeProductId ), async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryConsumableBalanceRemainingResult( XAsyncBlock *async, XStoreConsumableResult *consumableResult ) override
    {
        FIXME( "iface %p, async %p, consumableResult %p stub!\n", this, async, consumableResult );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreReportConsumableFulfillmentAsync( const XStoreContextHandle storeContextHandle, const char *storeProductId, UINT32 quantity, GUID trackingId, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, storeProductId %s, quantity %u, trackingId %s, async %p stub!\n", this, storeContextHandle, debugstr_a( storeProductId ), quantity, debugstr_guid( &trackingId ), async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreReportConsumableFulfillmentResult( XAsyncBlock *async, XStoreConsumableResult *consumableResult ) override
    {
        FIXME( "iface %p, async %p, consumableResult %p stub!\n", this, async, consumableResult );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreGetUserCollectionsIdAsync( const XStoreContextHandle storeContextHandle, const char *serviceTicket, const char *publisherUserId, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, serviceTicket %s, userId %s, async %p stub!\n", this, storeContextHandle, debugstr_a( serviceTicket ), debugstr_a( publisherUserId ), async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreGetUserCollectionsIdResultSize( XAsyncBlock *async, SIZE_T *size ) override
    {
        FIXME( "iface %p, async %p, size %p stub!\n", this, async, size );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreGetUserCollectionsIdResult( XAsyncBlock *async, SIZE_T size, char *result ) override
    {
        FIXME( "iface %p, async %p, size %Iu, result %p stub!\n", this, async, size, result );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreGetUserPurchaseIdAsync( const XStoreContextHandle storeContextHandle, const char *serviceTicket, const char *publisherUserId, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, serviceTicket %s, publisherUserId %s, async %p stub!\n", this, storeContextHandle, debugstr_a( serviceTicket ), debugstr_a( publisherUserId ), async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreGetUserPurchaseIdResultSize( XAsyncBlock *async, SIZE_T *size ) override
    {
        FIXME( "iface %p, async %p, size %p stub!\n", this, async, size );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreGetUserPurchaseIdResult( XAsyncBlock *async, SIZE_T size, char *result ) override
    {
        FIXME( "iface %p, async %p, size %Iu, result %p stub!\n", this, async, size, result );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryLicenseTokenAsync( const XStoreContextHandle storeContextHandle, const char **productIds, SIZE_T productIdsCount, const char *customDeveloperString, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, productIdsCount %p, idsCount %Iu, custom %s, async %p stub!\n", this, storeContextHandle, productIds, productIdsCount, debugstr_a( customDeveloperString ), async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryLicenseTokenResultSize( XAsyncBlock *async, SIZE_T *size ) override
    {
        FIXME( "iface %p, async %p, size %p stub!\n", this, async, size );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryLicenseTokenResult( XAsyncBlock *async, SIZE_T size, char *result ) override
    {
        FIXME( "iface %p, async %p, size %Iu, result %p stub!\n", this, async, size, result );
        return E_NOTIMPL;
    }

    HRESULT WINAPI __PADDING__() override
    {
        WARN( "iface %p padding function called! It's unknown what this function does.\n", this );
        return E_NOTIMPL;
    }

    HRESULT WINAPI __PADDING_2__() override
    {
        WARN( "iface %p padding function called! It's unknown what this function does.\n", this );
        return E_NOTIMPL;
    }

    HRESULT WINAPI __PADDING_3__() override
    {
        WARN( "iface %p padding function called! It's unknown what this function does.\n", this );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreShowPurchaseUIAsync( const XStoreContextHandle storeContextHandle, const char *storeId, const char *name, const char *extendedJsonData, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, storeId %s, name %s, extendedJsonData %s, async %p stub!\n", this, storeContextHandle, debugstr_a( storeId ), debugstr_a( name ), debugstr_a( extendedJsonData ), async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreShowPurchaseUIResult( XAsyncBlock *async ) override
    {
        FIXME( "iface %p, async %p stub!\n", this, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreShowRateAndReviewUIAsync( const XStoreContextHandle storeContextHandle, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, async %p stub!\n", this, storeContextHandle, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreShowRateAndReviewUIResult( XAsyncBlock *async, XStoreRateAndReviewResult *result ) override
    {
        FIXME( "iface %p, async %p, result %p stub!\n", this, async, result );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreShowRedeemTokenUIAsync( const XStoreContextHandle storeContextHandle, const char *token, const char **allowedStoreIds, SIZE_T allowedStoreIdsCount, BOOLEAN disallowCsvRedemption, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, token %s, allowedStoreIds %p, allowedStoreIdsCount %Iu, disallowCsvRedemption %d, async %p stub!\n", this, storeContextHandle, debugstr_a( token ), allowedStoreIds, allowedStoreIdsCount, disallowCsvRedemption, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreShowRedeemTokenUIResult( XAsyncBlock *async ) override
    {
        FIXME( "iface %p, async %p stub!\n", this, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryGameAndDlcPackageUpdatesAsync( const XStoreContextHandle storeContextHandle, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, async %p stub!\n", this, storeContextHandle, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryGameAndDlcPackageUpdatesResultCount( XAsyncBlock *async, UINT32 *count ) override
    {
        FIXME( "iface %p, async %p, count %p stub!\n", this, async, count );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryGameAndDlcPackageUpdatesResult( XAsyncBlock *async, UINT32 count, XStorePackageUpdate *packageUpdates ) override
    {
        FIXME( "iface %p, async %p, count %u, packageUpdates %p stub!\n", this, async, count, packageUpdates );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreDownloadPackageUpdatesAsync( XStoreContextHandle storeContextHandle, const char **packageIdentifiers, SIZE_T packageIdentifiersCount, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, packageIdentifiers %p, packageIdentifiersCount %Iu, async %p stub!\n", this, storeContextHandle, packageIdentifiers, packageIdentifiersCount, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreDownloadPackageUpdatesResult( XAsyncBlock *async ) override
    {
        FIXME( "iface %p, async %p stub!\n", this, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreDownloadAndInstallPackageUpdatesAsync( const XStoreContextHandle storeContextHandle, const char **packageIdentifiers, SIZE_T packageIdentifiersCount, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, packageIdentifiers %p, packageIdentifiersCount %Iu, async %p stub!\n", this, storeContextHandle, packageIdentifiers, packageIdentifiersCount, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreDownloadAndInstallPackageUpdatesResult( XAsyncBlock *async ) override
    {
        FIXME( "iface %p, async %p stub!\n", this, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreDownloadAndInstallPackagesAsync( const XStoreContextHandle storeContextHandle, const char **storeIds, SIZE_T storeIdsCount, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, storeIds %p, storeIdsCount %Iu, async %p stub!\n", this, storeContextHandle, storeIds, storeIdsCount, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreDownloadAndInstallPackagesResultCount( XAsyncBlock *async, UINT32 *count ) override
    {
        FIXME( "iface %p, async %p, count %p stub!\n", this, async, count );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreDownloadAndInstallPackagesResult( XAsyncBlock *async, UINT32 count, char **packageIdentifiers ) override
    {
        FIXME( "iface %p, async %p, count %u, packageIdentifiers %p stub!\n", this, async, count, packageIdentifiers );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryPackageIdentifier( const char *storeId, SIZE_T size, char *packageIdentifier ) override
    {
        FIXME( "iface %p, storeId %s, size %Iu, packageIdentifier %p stub!\n", this, debugstr_a( storeId ), size, packageIdentifier );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreRegisterGameLicenseChanged( XStoreContextHandle storeContextHandle, XTaskQueueHandle queue, void *context, XStoreGameLicenseChangedCallback *callback, XTaskQueueRegistrationToken *token ) override
    {
        FIXME( "iface %p, storeContextHandle %p, queue %p, context %p, callback %p, token %p stub!\n", this, storeContextHandle, queue, context, callback, token );
        return E_NOTIMPL;
    }

    BOOLEAN WINAPI XStoreUnregisterGameLicenseChanged( XStoreContextHandle storeContextHandle, XTaskQueueRegistrationToken token, BOOLEAN wait ) override
    {
        FIXME( "iface %p, storeContextHandle %p, token %p, wait %d stub!\n", this, storeContextHandle, &token, wait );
        return FALSE;
    }

    HRESULT WINAPI XStoreRegisterPackageLicenseLost( XStoreLicenseHandle storeLicenseHandle, XTaskQueueHandle queue, void *context, XStorePackageLicenseLostCallback *callback, XTaskQueueRegistrationToken *token) override
    {
        FIXME( "iface %p, storeLicenseHandle %p, queue %p, context %p, callback %p, token %p stub!\n", this, storeLicenseHandle, queue, context, callback, token );
        return E_NOTIMPL;
    }

    BOOLEAN WINAPI XStoreUnregisterPackageLicenseLost( XStoreLicenseHandle licenseHandle, XTaskQueueRegistrationToken token, BOOLEAN wait ) override
    {
        FIXME( "iface %p, licenseHandle %p, token %p, wait %d stub!\n", this, licenseHandle, &token, wait );
        return FALSE;
    }

    BOOLEAN WINAPI XStoreIsAvailabilityPurchasable( const XStoreAvailability availability ) override
    {
        FIXME( "iface %p, availability %p stub!\n", this, &availability );
        return FALSE;
    }

    HRESULT WINAPI XStoreAcquireLicenseForDurablesAsync( const XStoreContextHandle storeContextHandle, const char *storeId, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, storeId %s, async %p stub!\n", this, storeContextHandle, debugstr_a( storeId ), async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreAcquireLicenseForDurablesResult( XAsyncBlock *async, XStoreLicenseHandle *storeLicenseHandle ) override
    {
        FIXME( "iface %p, async %p, storeLicenseHandle %p stub!\n", this, async, storeLicenseHandle );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreShowAssociatedProductsUIAsync( const XStoreContextHandle storeContextHandle, const char *storeId, XStoreProductKind productKinds, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, storeId %s, productKinds %#x, async %p stub!\n", this, storeContextHandle, debugstr_a( storeId ), static_cast<UINT32>(productKinds), async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreShowAssociatedProductsUIResult( XAsyncBlock *async ) override
    {
        FIXME( "iface %p, async %p stub!\n", this, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreShowProductPageUIAsync( const XStoreContextHandle storeContextHandle, const char *storeId, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, storeId %s, async %p stub!\n", this, storeContextHandle, debugstr_a( storeId ), async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreShowProductPageUIResult( XAsyncBlock *async ) override
    {
        FIXME( "iface %p, async %p stub!\n", this, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryAssociatedProductsForStoreIdAsync( const XStoreContextHandle storeContextHandle, const char *storeId, XStoreProductKind productKinds, UINT32 maxItemsToRetrievePerPage, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, storeId %s, productKinds %#x, maxItemsToRetrievePerPage %u, async %p stub!\n", this, storeContextHandle, debugstr_a( storeId ), static_cast<UINT32>(productKinds), maxItemsToRetrievePerPage, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryAssociatedProductsForStoreIdResult( XAsyncBlock *async, XStoreProductQueryHandle *productQueryHandle ) override
    {
        FIXME( "iface %p, async %p, productQueryHandle %p stub!\n", this, async, productQueryHandle );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryPackageUpdatesAsync( XStoreContextHandle storeContextHandle, const char **packageIdentifiers, SIZE_T packageIdentifiersCount, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, packageIdentifiers %p, packageIdentifiersCount %Iu, async %p stub!\n", this, storeContextHandle, packageIdentifiers, packageIdentifiersCount, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryPackageUpdatesResultCount( XAsyncBlock *async, UINT32 *count ) override
    {
        FIXME( "iface %p, async %p, count %p stub!\n", this, async, count );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreQueryPackageUpdatesResult( XAsyncBlock *async, UINT32 count, XStorePackageUpdate *packageUpdates ) override
    {
        FIXME( "iface %p, async %p, count %u, packageUpdates %p stub!\n", this, async, count, packageUpdates );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreShowGiftingUIAsync( const XStoreContextHandle storeContextHandle, const char *storeId, const char *name, const char *extendedJsonData, XAsyncBlock *async ) override
    {
        FIXME( "iface %p, storeContextHandle %p, storeId %s, name %s, extendedJsonData %s, async %p stub!\n", this, storeContextHandle, debugstr_a( storeId ), debugstr_a( name ), debugstr_a( extendedJsonData ), async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XStoreShowGiftingUIResult( XAsyncBlock *async ) override
    {
        FIXME( "iface %p, async %p stub!\n", this, async );
        return E_NOTIMPL;
    }

private:
    LONG ref = 1;
};

static XStoreImpl g_x_store;

IXStoreImpl *x_store = static_cast<IXStoreImpl *>(&g_x_store);
