#include <iostream>

#include "include/RefBase.h"

using namespace std;
using namespace Rene;

class RBTester : public RefBase {
public:
    RBTester()
    {
        cout<<"RBTester ctor: "<<this<<endl;
    }

    ~RBTester()
    {
        cout<<"RBTester dtor: "<<this<<endl;
    }
};

int main()
{
    cout<<"-----------------------begin------------------------"<<endl;

    /*
    RBTester *pTester = new RBTester();
    cout<<"-------------0000---------------"<<endl;
    sp<RBTester> sp_rbt0;
    sp_rbt0 = pTester;
    cout<<"sp_rbt0 strong_ref: "<<sp_rbt0->getStrongCount()<<endl;
    //cout<<"sp_rbt0 weak_ref: "<<sp_rbt0->createWeak(NULL)->getWeakCount()<<endl;
    cout<<"sp_rbt0 weak_ref: "<<sp_rbt0->getWeakRefs()->getWeakCount()<<endl;
    sp_rbt0->printRefs();
    cout<<"-------------1111---------------"<<endl;
    sp<RBTester> sp_rbt1 = new RBTester();
    sp_rbt1 = sp_rbt0;
    sp_rbt1->printRefs();
    cout<<"-------------2222---------------"<<endl;
    sp<RBTester> sp_rbt2(sp_rbt0);
    sp_rbt2->printRefs();
    cout<<"-------------3333---------------"<<endl;
    // test ctor
    wp<RBTester> wp_rbt0 = pTester;
    sp_rbt0->printRefs();
    wp<RBTester> wp_rbt1 = wp_rbt0;
    sp_rbt0->printRefs();
    wp<RBTester> wp_rbt2 = sp_rbt0;
    sp_rbt0->printRefs();
    // test assignment
    wp<RBTester> wp_rbt3;
    wp_rbt3 = pTester;
    sp_rbt0->printRefs();
    wp<RBTester> wp_rbt4;
    wp_rbt4 = wp_rbt0;
    sp_rbt0->printRefs();
    wp<RBTester> wp_rbt5;
    wp_rbt4 = wp_rbt5;
    sp_rbt0->printRefs();
    */
    sp<RBTester> sp_tester = new RBTester();
    wp<RBTester> wp_tester0 = sp_tester;
    sp_tester.printRefs();
    wp<RBTester> wp_tester1 = new RBTester();
    sp<RBTester> promotion = wp_tester1.promote();
    if (promotion.get() != NULL)
    {
        cout<<"ok"<<endl;
    }
    else
    {
        cout<<"fail"<<endl;
    }
    //sp_wp_promotion = wp_tester.promote();
    //cout<<"promotion:"<<sp_wp_promotion.get()<<endl;
    //sp_tester->printRefs();
    //wp<RBTester> wp_rbt = new RBTester();
    //sp<RBTester> aa = wp_rbt.promote();
    //aa->printRefs();
    cout<<"-----------------------end------------------------"<<endl;
    return 0;
}
