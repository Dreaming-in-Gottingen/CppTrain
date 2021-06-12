#define NO_DEBUG 0
#define VERBOSE_INFO 0
#define WARNING_INFO 1  // when ctor or dtor
#define ERROR_INFO 1

#include <iostream>
#include "include/RefBase.h"

using namespace std;
using namespace Rene;

class tmp : public RefBase
{
public:
    tmp()
    {
        cout<<"tmp ctor"<<endl;
    }
    void printSth(void)
    {
        cout<<"hello,world"<<endl;
    }
protected:
    tmp(const tmp& other)
    {
    }
    tmp(const tmp* other)
    {
    }
    ~tmp()
    {
        cout<<"tmp dtor"<<endl;
    }

    virtual void onFirstRef(void)
    {
        cout<<"tmp onFirstRef"<<endl;
    }
};

void transfer_sp(sp<tmp> & spt)
{
    sp<tmp> sp_tmp(spt);
    sp_tmp.printRefs();
}

void transfer_ptr(tmp * ptr)
{
    sp<tmp> sp_tmp(ptr);
    sp_tmp.printRefs();
}

void transfer_obj(sp<tmp> obj)
{
    sp<tmp> sp_tmp(obj);
    sp_tmp.printRefs();
}

int main()
{
   tmp *pTmp = new tmp();
   cout<<"----------pTmp: "<<pTmp<<"------------"<<endl;
   sp<tmp> spTmp(pTmp);
   spTmp.printRefs();
   cout<<"----------spTmp: "<<&spTmp<<"------------"<<endl;
   sp<tmp> sp1Tmp(pTmp);
   spTmp.printRefs();
   spTmp->printSth();
   cout<<"----------sp1Tmp: "<<&sp1Tmp<<"------------"<<endl;
   cout<<"------------ptr------------"<<endl;
   transfer_ptr(pTmp);
   cout<<"------------sp ref------------"<<endl;
   transfer_sp(spTmp);
   cout<<"------------sp obj------------"<<endl;
   transfer_obj(spTmp);
   cout<<"------------print refs------------"<<endl;
   spTmp.printRefs();
   cout<<"------------------------"<<endl;
//    sp<tmp> sp2Tmp(pTmp);
//    spTmp.printRefs();
//    cout<<"-------------------------------"<<endl;
//    sp<tmp> sp3Tmp(spTmp);
//    spTmp.printRefs();
//    cout<<"-------------------------------"<<endl;
//    sp<int> s;
//    spTmp->printSth();

}
