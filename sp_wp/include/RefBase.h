#include <iostream>
#include <assert.h>

#ifndef NO_DEBUG
#define NO_DEBUG 1
#endif

namespace Rene {


//------------------------------------------------------------------------------
template <class T>
class LightRefBase
{
public:
    inline LightRefBase() : mCount(0)
    {
#if NO_DEBUG==0
        std::cout<<"LightRefBase ctor: "<<this<<std::endl;
#endif
    }
    inline void incStrong(const void* id)
    {
        __sync_fetch_and_add(&mCount, 1);
    }
    inline void decStrong(const void* id)
    {
        if(__sync_fetch_and_sub(&mCount, 1) == 1)
        {
            delete static_cast<const T*>(this);
        }
    }
    inline int getStrongCount() const
    {
        return mCount;
    }

protected:
    inline ~LightRefBase()
    {
#if NO_DEBUG==0
        std::cout<<"LightRefBase dtor: "<<this<<std::endl;
#endif
    }

private:
    mutable volatile int mCount;
};

//------------------------------------------------------------------------------
class RefBase {
public:
#define INITIAL_STRONG_VALUE (1<<28)
    inline void incStrong(const void* id) const
    {
        weakref_impl *ref = mpRefs;
        ref->incWeak(id);

        ref->addStrongRef(id);
        const int c = __sync_fetch_and_add(&ref->mStrong, 1);
        assert(c > 0);

        //not first ref!
        if (c != INITIAL_STRONG_VALUE) return;

        //first ref, target(sub class of RefBase) use onFirstRef to init sth
        __sync_fetch_and_sub(&ref->mStrong, INITIAL_STRONG_VALUE);
        //const_cast<RefBase*>(this)->onFirstRef();
        ref->mBase->onFirstRef();
    }

    inline void decStrong(const void* id) const
    {
        weakref_impl *refs = mpRefs; 
        refs->removeStrongRef(id);
        const int c = __sync_fetch_and_sub(&refs->mStrong, 1);
        assert(c >= 1);
        if (c == 1)
        {
            //const_cast<RefBase*>(this)->onLastStrongRef(id);
            refs->mBase->onLastStrongRef(id);
            if ((refs->mFlags & OBJECT_LIFETIME_MASK) == OBJECT_LIFETIME_STRONG)
            {
#if NO_DEBUG==0 && WARNING_INFO==1
                std::cout<<"decStrong() delete RefBase: "<<this<<std::endl;
#endif
                delete this;
            }
        }
        refs->decWeak(id);
    }

    inline int getStrongCount() const
    {
        return mpRefs->mStrong;
    }

    void dumpRefs() const
    {
        mpRefs->printRefs();
    }

    class weakref_type
    {
    public:
        RefBase * refBase() const
        {
            return NULL;
        }

        void incWeak(const void* id)
        {
            weakref_impl * impl = static_cast<weakref_impl*>(this);
            impl->addWeakRef(id);
            int c = __sync_fetch_and_add(&impl->mWeak, 1);
            assert(c >= 0);
#if NO_DEBUG==0 && VERBOSE_INFO==1
            std::cout<<"incWeak() to ["<<impl->mWeak<<"]!"<<std::endl;
#endif
        }
    
        void decWeak(const void* id)
        {
            weakref_impl * impl = static_cast<weakref_impl*>(this);
            impl->removeWeakRef(id);
            int c = __sync_fetch_and_sub(&impl->mWeak, 1);
            assert(c >= 1);
            if (c > 1)
            {
#if NO_DEBUG==0 && VERBOSE_INFO==1
                std::cout<<"decWeak(), at last, mWeak="<<impl->mWeak<<std::endl;
#endif
                return;
            }
            else
            {
                if ((impl->mFlags & OBJECT_LIFETIME_MASK) == OBJECT_LIFETIME_STRONG)
                {
                    if (impl->mStrong == INITIAL_STRONG_VALUE)
                    {
                        assert(impl->mWeak == 0);
                        delete impl->mBase;
                    }
                    else
                    {
#if NO_DEBUG==0 && VERBOSE_INFO==1
                        std::cout<<"mStrong==0 && mWeak==0, delete weakref_impl!"<<std::endl;
#endif
                        assert(impl->mStrong == 0 && impl->mWeak == 0);
                        delete impl;
                    }
                }
                else
                {
                    impl->mBase->onLastWeakRef(id);
                    if ((impl->mFlags & OBJECT_LIFETIME_MASK) == OBJECT_LIFETIME_WEAK)
                        delete impl->mBase;
                }
            }
        }

        bool attemptIncStrong(const void* id)
        {
            incWeak(id);
            weakref_impl* const impl = static_cast<weakref_impl*>(this);
            int curCount = impl->mStrong;
            assert(curCount>=0);

            while(curCount > 0 && curCount != INITIAL_STRONG_VALUE)
            {
                // the common case of promoting a weak-ref from an existing srong reference
                if (__sync_bool_compare_and_swap(&impl->mStrong, curCount, curCount+1)) break;
                // mStong changed by other thread at the same moment, re-assert our situation
                curCount = impl->mStrong;
            }

            if (curCount<=0 || curCount==INITIAL_STRONG_VALUE)
            {
                if ((impl->mFlags&OBJECT_LIFETIME_WEAK) == OBJECT_LIFETIME_STRONG)
                {
                    // this object has a "normal" life-time, i.e: it gets destroyed when the last strong-ref goes away
                    if (curCount <= 0)
                    {
                        // the last strong-ref got released.
                        decWeak(id);
                        return false;
                    }

                    // curCount == INITIAL_STRONG_VALUE, this means that there was never a strong-ref to it so far,
                    // so we try to promote this object, and we need to do that atomically.
                    while (curCount > 0)
                    {
                        if (__sync_bool_compare_and_swap(&impl->mStrong, curCount, curCount+1))
                        {
                            break;
                        }
                        // strong count changed on us by another thread.
                        curCount = impl->mStrong;
                    }

                    if (curCount <= 0)
                    {
                        // promote failed, other threads destroyed us in the meantime.
                        // we hardly meet this case.
                        decWeak(id);
                        return false;
                    }
                }
                else
                {
                    // this object has an "extended" life-time, i.e.: it can be revived from a weak-reference only;
                    // ask the object's implementation if it agree to be revived.
                    if (!impl->mBase->onIncStrongAttempted(FIRST_INC_STRONG, id))
                    {
                        decWeak(id);
                        return false;
                    }
                    curCount = __sync_fetch_and_add(&impl->mStrong, 1);
                }

                // strong-ref increased by someone else, the implementor of onIncStrongAttempted()
                // is holding an unneed reference, so call onLastStrongRef() to remove it.
                // Note that we MUST NOT do this if we are acquiring the first reference.
                // we hardly meet this case, only when more one thread are acquiring ref.
                // curCount==INITIAL_STRONG_VALUE+1 mostly 
                if (curCount>0 && curCount<INITIAL_STRONG_VALUE)
                {
                    impl->mBase->onLastStrongRef(id);
                }
            }

            impl->addStrongRef(id);

            std::cout<<"attempIncStrong of ["<<this<<"] from ["<<id<<"]: cnt=["<<curCount<<"]"<<std::endl;

            curCount = impl->mStrong;
            while (curCount >= INITIAL_STRONG_VALUE)
            {
                assert(curCount > INITIAL_STRONG_VALUE);
                if (__sync_bool_compare_and_swap(&impl->mStrong, curCount, curCount-INITIAL_STRONG_VALUE))
                {
                    break;
                }
                curCount = impl->mStrong;
            }

            return true;
        }

        bool attemptIncWeak(const void* id)
        {
        
        }

        int getWeakCount() const
        {
            return static_cast<const weakref_impl*>(this)->mWeak;
        }
    };

    // careful: will inc mWeak!
    // only used by Binder.cpp
    weakref_type* createWeak(const void* id) const
    {
        mpRefs->incWeak(id);   
        return mpRefs;
    }

    weakref_type* getWeakRefs() const
    {
        return mpRefs;
    }

protected:
    RefBase()
        : mpRefs(new weakref_impl(this))
    {
#if NO_DEBUG==0 && WARNING_INFO==1
        std::cout<<"RefBase() ctor, create mpRefs["<<mpRefs<<"] for real obj["<<this<<"]"<<std::endl;
#endif
    }

    virtual ~RefBase()
    {
#if NO_DEBUG==0 && WARNING_INFO==1
        std::cout<<"RefBase dtor"<<std::endl;
#endif
        if (mpRefs->mStrong == INITIAL_STRONG_VALUE)
        {
#if NO_DEBUG==0 && VERBOSE_INFO==1
            std::cout<<"RefBase dtor, mStrong=Default"<<std::endl;
#endif
            assert(mpRefs->mWeak == 0);
            delete mpRefs;
        }
        else
        {
            if ((mpRefs->mFlags & OBJECT_LIFETIME_MASK) != OBJECT_LIFETIME_STRONG)
            {
                if(mpRefs->mWeak == 0)
                {
#if NO_DEBUG==0 && VERBOSE_INFO==1
                    std::cout<<"RefBase dtor, mWeak=0"<<std::endl;
#endif
                    assert(mpRefs->mStrong==0 && mpRefs->mWeak==0);
                    delete mpRefs;
                }
            }
        }
    }

    enum {
        OBJECT_LIFETIME_STRONG  = 0x0000,
        OBJECT_LIFETIME_WEAK    = 0x0001,
        OBJECT_LIFETIME_MASK    = 0x0001
    };
    void extendObjectLifttime(int mode)
    {
    }

    enum {
        FIRST_INC_STRONG = 0x0001
    };

    virtual void onFirstRef()
    {
#if NO_DEBUG==0 && VERBOSE_INFO==1
        std::cout<<"RefBase onFirstRef"<<std::endl;
#endif
    }

    virtual void onLastStrongRef(const void* id)
    {
    }

    virtual bool onIncStrongAttempted(int flags, const void* id)
    {
        return (flags&FIRST_INC_STRONG) ? true : false;
    }

    virtual void onLastWeakRef(const void* id)
    {
    }


private:
    //friend class weakref_type;

    class weakref_impl : public weakref_type
    {
    public:
        volatile int mStrong;
        volatile int mWeak;
        volatile int mFlags;
        RefBase * const mBase;

        weakref_impl(RefBase * base)
            : mStrong(INITIAL_STRONG_VALUE)
            , mWeak(0)
            , mFlags(0)
            , mBase(base)
        {
#if NO_DEBUG==0 && WARNING_INFO==1
            std::cout<<"weakref_impl() ctor!"<<std::endl;
#endif
        }
        ~weakref_impl()
        {
#if NO_DEBUG==0 && WARNING_INFO==1
             std::cout<<"weakref_impl() dtor"<<std::endl;
#endif
        }
    
        void addStrongRef(const void* id)
        {
#if NO_DEBUG==0 && VERBOSE_INFO==1
             std::cout<<__func__<<", id="<<id<<std::endl;
#endif
        }
        void removeStrongRef(const void* id)
        {
#if NO_DEBUG==0 && VERBOSE_INFO==1
             std::cout<<__func__<<", id="<<id<<std::endl;
#endif
        }
        void addWeakRef(const void* id)
        {
#if NO_DEBUG==0 && VERBOSE_INFO==1
             std::cout<<__func__<<", id="<<id<<std::endl;
#endif
        }
        void removeWeakRef(const void* id)
        {
#if NO_DEBUG==0 && VERBOSE_INFO==1
             std::cout<<__func__<<", id="<<id<<std::endl;
#endif
        }
        void printRefs() const
        {
             std::cout<<"refInfo -> mStrong: ["<<mStrong<<"], mWeak: ["<<mWeak<<"]"<<std::endl;
        }
    };

    RefBase(const RefBase& o);
    RefBase& operator=(const RefBase& o);

    weakref_impl* const mpRefs;
};


//------------------------------------------------------------------------------
template <class T>
class sp
{
public:
    inline sp() : m_ptr(0) {}
    sp(T* other) : m_ptr(other)
    {
#if NO_DEBUG==0 && WARNING_INFO==1
        std::cout<<"sp(T*) ctor: "<<this<<std::endl;
#endif
        if (other) other->incStrong(this);
    }
    sp(const sp<T>& other) : m_ptr(other.m_ptr)
    {
#if NO_DEBUG==0 && WARNING_INFO==1
        std::cout<<"sp(sp<T>&) ctor: "<<this<<std::endl;
#endif
        if (m_ptr) m_ptr->incStrong(this);
    }

    void printRefs() const
    {
        if (m_ptr)
        {
            std::cout<<"dump m_ptr["<<m_ptr<<"] ref info..."<<std::endl;
            m_ptr->dumpRefs();
        }
        else
        {
            std::cout<<"m_ptr has been deleted!"<<std::endl;
        }
    }

    ~sp() 
    {
#if NO_DEBUG==0 && WARNING_INFO==1
        std::cout<<"~sp() dtor: "<<this<<std::endl;
#endif
        if (m_ptr) m_ptr->decStrong(this);
    }

    // assignment
    sp<T>& operator = (T* other)
    {
#if NO_DEBUG==0 && WARNING_INFO==1
        std::cout<<"assignment, operator = (T* other), other=["<<other<<"]"<<std::endl;
#endif
        if (other)
            other->incStrong(this);
        if (m_ptr)
            m_ptr->decStrong(this);
        m_ptr = other;
        return *this;
    }
    sp<T>& operator = (const sp<T>& other)
    {
#if NO_DEBUG==0 && WARNING_INFO==1
        std::cout<<"assignment, operator = (const sp<T>& other), other.get()=["<<other.get()<<"]"<<std::endl;
#endif
        T* otherPtr = other.m_ptr;
        if (otherPtr)
            otherPtr->incStrong(this);
        if (m_ptr)
            m_ptr->decStrong(this);
        m_ptr = otherPtr;
        return *this;
    }

    void clear()
    {
        if (m_ptr) {
            m_ptr->decStrong(this);
            m_ptr = NULL;
        }
    }

    void set_pointer(T* ptr)
    {
        m_ptr = ptr;
    }

    inline T& operator* () const
    {
        return *m_ptr;
    }
    inline T* operator-> () const
    {
        return m_ptr;
    }
    inline T* get() const
    {
        return m_ptr;
    }

private:
    template<typename Y> friend class wp;
    T* m_ptr;
};

//------------------------------------------------------------------------------
#define COMPARE_WEAK(_op_)                          \
inline bool operator _op_ (const sp<T>& o) const {  \
    return m_ptr _op_ o.m_ptr;                      \
}                                                   \
inline bool operator _op_ (const T* o) const {      \
    return m_ptr _op_ o;                            \
}                                                   \

//------------------------------------------------------------------------------
template <typename T>
class wp
{
public:
    typedef RefBase::weakref_type weakref_type;

    // ctor
    inline wp() : m_ptr(0)
    {
#if NO_DEBUG==0 && WARNING_INFO==1
        std::cout<<"wp() ctor"<<std::endl;
#endif
    }

    wp(T* other) : m_ptr(other)
    {
//#if NO_DEBUG==0 && WARNING_INFO==1
        std::cout<<"wp(T* other) ctor, other:["<<other<<"]"<<std::endl;
//#endif
        if (other) m_refs = other->createWeak(this);
    }
    wp(const wp<T>& other)
        : m_ptr(other.m_ptr), m_refs(other.m_refs)
    {
//#if NO_DEBUG==0 && WARNING_INFO==1
        std::cout<<"wp(wp<T>& other) ctor, m_ptr:["<<m_ptr<<"], m_refs:["<<m_refs<<"]"<<std::endl;
//#endif
        if (m_ptr) m_refs->incWeak(this);
    }
    wp(const sp<T>& other)
        : m_ptr(other.m_ptr)
    {
//#if NO_DEBUG==0 && WARNING_INFO==1
        std::cout<<"wp(sp<T>& other) ctor, m_ptr:["<<m_ptr<<"]"<<std::endl;
//#endif
        if (m_ptr) m_refs = m_ptr->createWeak(this);
    }

    // dtor
    ~wp()
    {
        if (m_ptr) m_refs->decWeak(this);
    }

    // assignment
    wp& operator = (T* other)
    {
//#if NO_DEBUG==0 && WARNING_INFO==1
        std::cout<<"operator = (T* other), other:["<<other<<"]"<<std::endl;
//#endif
        weakref_type* newRefs = other ? other->createWeak(this) : 0;
        if (m_ptr) m_refs->decWeak(this);
        m_ptr = other;
        m_refs = newRefs;
        return *this;
    }
    wp& operator = (const wp<T>& other)
    {
//#if NO_DEBUG==0 && WARNING_INFO==1
        std::cout<<"operator = (wp<T>& other), other.m_ptr:["<<other.m_ptr<<"]"<<std::endl;
//#endif
        weakref_type* otherRefs = other.m_refs;
        T* otherPtr(other.m_ptr);
        if (otherPtr) otherRefs->incWeak(this);
        if (m_ptr) m_refs->decWeak(this);
        m_ptr = otherPtr;
        m_refs = otherRefs;
        return *this;
    }
    wp& operator = (const sp<T>& other)
    {
//#if NO_DEBUG==0 && WARNING_INFO==1
        std::cout<<"operator = (sp<T>& other), other.m_ptr:["<<other.m_ptr<<"]"<<std::endl;
//#endif
        weakref_type* newRefs = other!=NULL ? other->createWeak(this) : 0;
        T* otherPtr(other.m_ptr);
        if (m_ptr) m_refs->decWeak(this);
        m_ptr = other;
        m_refs = newRefs;
        return *this;
    }

    // only used by Parcel
    void set_object_and_refs(T* other, weakref_type* refs)
    {
        if (other) refs->incWeak(this);
        if (m_ptr) m_refs->decWeak(this);
        m_ptr = other;
        m_refs = refs;
    }

    // promote to sp
    sp<T> promote() const
    {
        sp<T> result;
        if (m_ptr && m_refs->attemptIncStrong(&result)) {
            result.set_pointer(m_ptr);
        }
        return result;
    }

    // reset
    void clear()
    {
        if (m_ptr) {
            m_refs->decWeak(this);
            m_ptr = 0;
        }
    }

    // accessors
    inline weakref_type* get_refs() const {return m_refs;}
    inline T* unsafe_get() const {return m_ptr;}

    // operators
    COMPARE_WEAK(==)
    COMPARE_WEAK(!=)
    COMPARE_WEAK(>)
    COMPARE_WEAK(<)
    COMPARE_WEAK(<=)
    COMPARE_WEAK(>=)

    // operators, param for wp<T>
    inline bool operator == (const wp<T>& o) const {
        return (m_ptr==o.m_ptr) && (m_refs==o.m_refs);
    }
    // hardly use
    inline bool operator > (const wp<T>& o) const {
        return (m_ptr==o.m_ptr) ? (m_refs>o.m_refs) : (m_ptr>o.m_ptr);
    }
    inline bool operator < (const wp<T>& o) const {
        return (m_ptr==o.m_ptr) ? (m_refs<o.m_refs) : (m_ptr<o.m_ptr);
    }
    inline bool operator != (const wp<T>& o) const {return m_refs != o.m_refs;}
    inline bool operator <= (const wp<T>& o) const {return !operator > (o);}
    inline bool operator >= (const wp<T>& o) const {return !operator < (o);}

private:
    template<typename Y> friend class sp;
    //template<typename Y> friend class wp;

    T*              m_ptr;
    weakref_type*   m_refs;
};


};
