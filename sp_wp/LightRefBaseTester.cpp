#include <iostream>

#include "include/RefBase.h"

using namespace std;
using namespace Rene;

class LRBTester : public LightRefBase<LRBTester>
{
public:
    LRBTester()
    {
        cout<<"LightRefBaseTester ctor!"<<endl;
    }

//protected:
    ~LRBTester()
    {
        cout<<"LightRefBaseTester dtor!"<<endl;
    }
};

int main()
{
    cout<<"--------begin--------"<<endl;

    LRBTester *pTester = new LRBTester();
    cout<<"0000. mCount:"<<pTester->getStrongCount()<<endl;
    sp<LRBTester> sp_lrb = pTester;
    cout<<"1111. mCount:"<<sp_lrb->getStrongCount()<<endl;
    sp_lrb->decStrong(NULL);
    cout<<"2222. mCount:"<<sp_lrb->getStrongCount()<<endl;

    cout<<"--------end--------"<<endl;

    return 0;
}

