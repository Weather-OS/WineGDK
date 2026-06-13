/*
 * LocklessQueue Implementation
 *  From https://github.com/microsoft/libHttpClient
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
#ifndef __LOCKLESSQUEUE_H__
#define __LOCKLESSQUEUE_H__

#include "../../../private.h"
#include "SpinLock.h"

/*****************************************************************************
 
 LocklessQueue is a lock free queue that can take any type as a payload.
 Internally it maintains a demand-allocated block of contiguous nodes that
 are used as the nodes of a linked list.  Data pushed into the queue uses a
 copy, and popping data copies that same data out to your variable.
 
 An initial block size is created up front and the size of the block can be customized
 via the constructor.  If the queue runs out of space, it can create new blocks.  This process
 is also lock / wait free, although it has an increassed "push" cost as the new block
 is allocated and initialized. If there is not enough memory to create a new block
 push_back returns false.
 
 N.B. LocklessQueue MUST be either dynamically allocated or stack allocated as a
 local variable.  It cannot be used as a member variable in a class or structure
 due to alignment restrictions. It can fail spectacularly if not aligned correctly.
 
 Typical Usage:
 
 struct MyPayload
 {
    uint32_t someData;
    // etc...
 };
 
 LocklessQueue<MyPayload*> queue;
 
 MyPayload p = new MyPayload;
 p->someData = 16;
 queue.push_back(p);
 
 if (queue.pop_front(p))
 {
    printf("Value: %d\r\n", p->someData);
    delete p;
 }
 
 
 LocklessQueue is based on the the algorithm described in this paper:

 Simple, Fast, and Practical Non-Blocking and Blocking Concurrent Queue Algorithms

 Maged M. Michael Michael L. Scott
 Department of Computer Science University of Rochester Rochester, NY 14627-0226 fmichael,scottg@cs.rochester.ed

 http://cs.rochester.edu/~scott/papers/1996_PODC_queues.pdf

 In addition to the above algorithm this variant introduces a heap of nodes.  This helps keep the memory
 contiguous and allows for single 64 bit swaps on 64 bit machines (as we don't need to store pointers).

 ******************************************************************************/

template <typename TData>
class alignas(8) LocklessQueue
{
public:
    LocklessQueue(_In_ uint32_t blockSize = 0x400) noexcept :
        m_localHeap(*this),
        m_heap(m_localHeap),
        m_activeList(*this),
        m_blockCache(nullptr),
        m_lock{}
    {
        m_localHeap.init(blockSize);
        Initialize();
    }

    LocklessQueue(_In_ LocklessQueue& shared) noexcept :
        m_localHeap(*this),
        m_heap(shared.m_heap),
        m_activeList(*this),
        m_blockCache(nullptr),
        m_lock{}
    {
        Initialize();
    }

    ~LocklessQueue() noexcept
    {
        if (&m_heap != &m_localHeap)
        {
            TData data;
            while(pop_front(data)) {}

            Address dummy = m_activeList.head();
            Node* node = to_node(dummy);
            m_heap.free(node, dummy);
        }
    }

    bool empty() noexcept
    {
        return m_activeList.empty();
    }

    bool reserve_node(_Out_ uint64_t& address) noexcept
    {
        Address a;
        Node* n = m_heap.alloc(a);

        if (n != nullptr)
        {
            address = a;
            return true;
        }
        else
        {
            address = 0;
            return false;
        }
    }

    void free_node(_In_ uint64_t address) noexcept
    {
        Address a;
        a = address;
        Node* n = to_node(a);
        m_heap.free(n, a);
    }

    bool push_back(_In_ const TData& data)
    {
        TData copy = data;
        return move_back(std::move(copy));
    }

    bool push_back(_In_ TData&& data) noexcept
    {
        return move_back(std::move(data));
    }

    void push_back(_In_ const TData& data, _In_ uint64_t address)
    {
        TData copy = data;
        move_back(std::move(copy), address);
    }

    void push_back(_In_ TData&& data, _In_ uint64_t address) noexcept
    {
        move_back(std::move(data), address);
    }

    bool pop_front(_Out_ TData& data) noexcept
    {
        Address address;
        Node* node = m_activeList.pop(address);
        
        if (node != nullptr)
        {
            data = std::move(node->data);
            node->data = TData {};
            m_heap.free(node, address);
            return true;
        }
        
        return false;
    }

    bool pop_front(_Out_ TData& data, _Out_ uint64_t& address) noexcept
    {
        Address a;
        Node* node = m_activeList.pop(a);
        
        if (node != nullptr)
        {
            data = std::move(node->data);
            node->data = TData {};
            address = a;
            return true;
        }
        
        return false;
    }

    template <typename TCallback>
    void remove_if(TCallback callback)
    {
        LocklessQueue<TData> retain(*this);
        TData entry;
        uint64_t address;

        SpinLock lock(m_lock);

        while (pop_front(entry, address))
        {
            if (!callback(entry, address))
            {
                retain.move_back(std::move(entry), address);
            }
        }

        while (retain.pop_front(entry, address))
        {
            move_back(std::move(entry), address);
        }
    }

private:
    struct Address
    {
        uint64_t index : 32;
        uint64_t block : 16;
        uint64_t aba   : 16;
        
        inline bool operator == (_In_ const Address& other) const
        {
            uint64_t v = *this;
            uint64_t ov = other;
            return v == ov;
        }

        inline bool operator != (_In_ const Address& other) const
        {
            uint64_t v = *this;
            uint64_t ov = other;
            return v != ov;
        }

        inline operator uint64_t () const
        {
            uint64_t v;
            memcpy(&v, this, sizeof(v));
            return v;
        }

        inline Address& operator = (_In_ uint64_t v)
        {
            memcpy(this, &v, sizeof(v));
            return *this;
        }
    };

    // This does not work on i686 targets.
    //static_assert(sizeof(Address) == sizeof(uint64_t), "LocklessQueue Address field must be 64 bits exactly");

    struct alignas(8) Node
    {
        std::atomic<Address> next;
        TData data;
    };

    struct alignas(8) Block
    {
        std::atomic<Block*> next;
        Node* nodes;
        uint32_t id;
        uint32_t padding;
    };

    class List
    {
    public:

        List(_In_ LocklessQueue& owner) :
            m_owner(owner)
        {}

        void init(_In_ Address dummy, _In_ Address end) noexcept
        {
            m_head = dummy;
            m_tail = dummy;
            m_end = end;
        }

        inline Address end() { return m_end; }
        inline Address head() { return m_head.load(); }

        bool empty() noexcept
        {
            Address head = m_head.load();
            Address tail = m_tail.load();
            Address next = m_owner.to_node(head)->next;
            
            if (head == m_head.load() &&
                head == tail &&
                next == m_end)
            {
                return true;
            }
            
            return false;
        }

        inline void push(_In_ Node* node, _In_ Address address) noexcept
        {
            node->next = m_end;
            address.aba++;
            push_range(address, address);
        }

        void push_range(_In_ Address beginAddress, _In_ Address tailAddress) noexcept
        {
            while(true)
            {
                Address tail = m_tail.load();
                Node* tailNode = m_owner.to_node(tail);
                Address tailNext = tailNode->next.load();
                
                if (tail == m_tail.load())
                {
                    if (tailNext == m_end)
                    {
                        if (tailNode->next.compare_exchange_strong(tailNext, beginAddress))
                        {
                            m_tail.compare_exchange_strong(tail, tailAddress);
                            break;
                        }
                    }
                    else
                    {
                        m_tail.compare_exchange_strong(tail, tailNext);
                    }
                }
            }
        }

        Node* pop(_Out_ Address& address) noexcept
        {
            while (true)
            {
                Address head = m_head.load();
                Address tail = m_tail.load();
                Node* headNode = m_owner.to_node(head);
                Address next = headNode->next.load();

                if (head == m_head.load())
                {
                    if (head == tail)
                    {
                        if (next == m_end)
                        {
                            address = m_end;
                            return nullptr;
                        }
                        m_tail.compare_exchange_strong(tail, next);
                    }
                    else
                    {
                        TData data = m_owner.to_node(next)->data;

                        if (m_head.compare_exchange_strong(head, next))
                        {
                            headNode->data = std::move(data);
                            address = head;
                            return headNode;
                        }
                    }
                }
            }
        }

    private:

        LocklessQueue& m_owner;
        std::atomic<Address> m_head;
        std::atomic<Address> m_tail;
        Address m_end;
    };

    class Heap
    {
    public:
        Heap(_In_ LocklessQueue& owner) :
            m_freeList(owner)
        {}

        ~Heap()
        {
            Block* block = m_blockList;
            while(block != nullptr)
            {
                Block* d = block;
                block = block->next;
                delete[] d->nodes;
                delete d;
            }
        }

        void init(_In_ uint32_t blockSize) noexcept
        {
            if (blockSize < 0x40)
            {
                blockSize = 0x40;
            }

            m_blockSize = blockSize;

            while(!allocate_block() && m_blockSize > 0x40)
            {
                m_blockSize = m_blockSize >> 2;
            }
        }

        inline Address end() { return m_freeList.end(); }

        Node* to_node(_Inout_ std::atomic<Block*>& blockCache, _In_ const Address& address)
        {
            Block* block = blockCache.load();

            if (block == nullptr || block->id != address.block)
            {
                for(block = m_blockList; block != nullptr; block = block->next)
                {
                    if (block->id == address.block)
                    {
                        blockCache = block;
                        break;
                    }
                }
            }

            return &block->nodes[address.index];
        }

        void free(_In_ Node* node, _In_ Address address) noexcept
        {
            m_freeList.push(node, address);
        }

        Node* alloc(_Out_ Address& address) noexcept
        {
            Node* node;

            do
            {
                node = m_freeList.pop(address);
                if (node == nullptr && !allocate_block())
                {
                    break;
                }
            } while(node == nullptr);

            return node;
        }

    private:

        std::atomic<uint32_t> m_blockCount = { 0 };
        uint32_t m_blockSize = 0;
        Block* m_blockList = nullptr;
        List m_freeList;

        bool allocate_block() noexcept
        {
            uint32_t blockId = m_blockCount.fetch_add(1) + 1;

            if (blockId > 0xFFFF)
            {
                return false;
            }

            Block* block = new (std::nothrow) Block;
            if (block == nullptr)
            {
                return false;
            }

            block->nodes = new (std::nothrow) Node[m_blockSize];
            if (block->nodes == nullptr)
            {
                delete block;
                return false;
            }

            block->id = blockId;        
            block->next = nullptr;


            Address prev{};
            for (uint32_t index = 0; index < m_blockSize; index++)
            {
                block->nodes[index].next = prev;
                prev.block = static_cast<uint16_t>(block->id);
                prev.index = index;
                prev.aba = 0;
            }

            uint32_t startIndex = 0;

            if (m_blockList == nullptr)
            {
                Address end{};
                Address a{};
                a.block = static_cast<uint16_t>(block->id);

                block->nodes[0].next = end;
                block->nodes[1].next = end;
                startIndex = 1;
                m_blockList = block;
                m_freeList.init(a, end);
            }
            else
            {
                Block* tail = m_blockList;
                Block* next = tail->next.load();

                while(true)
                {
                    while(next != nullptr)
                    {
                        tail = next;
                        next = tail->next.load();
                    }

                    Block* empty = nullptr;

                    if(tail->next.compare_exchange_strong(empty, block))
                    {
                        break;
                    }

                    next = tail->next.load();
                }
            }

            Address rangeBegin{};
            Address rangeEnd{};

            rangeBegin.block = rangeEnd.block = static_cast<uint16_t>(block->id);
            rangeBegin.index = m_blockSize - 1;
            rangeEnd.index = startIndex;
            m_freeList.push_range(rangeBegin, rangeEnd);

            return true;
        }
    };

    Heap m_localHeap;
    Heap& m_heap;
    List m_activeList;
    std::atomic<Block*> m_blockCache;
    std::atomic_flag m_lock;

    bool move_back(_In_ TData&& data) noexcept
    {
        Address address;
        Node* node = m_heap.alloc(address);

        if (node != nullptr)
        {
            node->data = std::move(data);
            m_activeList.push(node, address);
            return true;
        }

        return false;
    }

    void move_back(_In_ TData&& data, _In_ uint64_t address) noexcept
    {
        Address a;
        a = address;
        Node* n = to_node(a);
        n->data = std::move(data);
        m_activeList.push(n, a);
    }

    void Initialize()
    {
        Address end = m_heap.end();
        end.index++;

        Address dummy;
        Node* n = m_heap.alloc(dummy);

        if (n != nullptr)
        {
            n->next = end;
        }
        else
        {
            dummy = end;
        }

        m_activeList.init(dummy, end);
    }

    Node* to_node(_In_ const Address& address)
    {
        return m_heap.to_node(m_blockCache, address);
    }
};

#endif
