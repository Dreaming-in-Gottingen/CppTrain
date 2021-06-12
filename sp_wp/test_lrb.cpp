//#define NO_DEBUG 0

#include <iostream>
#include "include/RefBase.h"

using namespace std;
using namespace Rene;

class tmp : public LightRefBase<tmp>
{
public:
    tmp()
    {
        cout<<"tmp ctor"<<endl;
    }
    ~tmp()
    {
        cout<<"tmp dtor"<<endl;
    }
};

int main()
{
    tmp *pTmp = new tmp();
    cout<<"pTmp: "<<pTmp<<endl;
    sp<tmp> sp1_tmp(pTmp);
    sp<tmp> sp2_tmp(sp1_tmp);
    sp<tmp> sp3_tmp(sp1_tmp);
    cout<<"print ref cnt="<<pTmp->getStrongCount()<<endl;
    {
        sp<tmp> sp4_tmp(sp1_tmp);
        cout<<"print ref cnt="<<pTmp->getStrongCount()<<endl;
    }
    cout<<"print ref cnt="<<pTmp->getStrongCount()<<endl;

    return 0; 
}
