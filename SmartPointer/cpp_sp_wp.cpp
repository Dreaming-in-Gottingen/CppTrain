#include <iostream>
#include <memory>
#include <string>

using namespace std;

class Person {
public:
    Person() {
        cout<<"Person ctor"<<endl;
    }

    Person(const string &alias): name(alias) {
        cout<<"Person ctor for "<<name.c_str()<<endl;
    }

    ~Person() {
        cout<<"Person dtor for "<<name.c_str()<<endl;
    }

    void setFather(shared_ptr<Person> &p) {
        father = p;
    }

    void setSon(shared_ptr<Person> &p) {
        son = p;
    }

    void printName() {
        cout<<"name: "<<name.c_str()<<endl;
    }

private:
    const string name;
    shared_ptr<Person> father;
    shared_ptr<Person> son;
};

void test0()
{
    cout<<"---------test0 normal release begin---------"<<endl;

    shared_ptr<Person> sp_pf(new Person("zjz"));
    shared_ptr<Person> sp_ps(new Person("zcx"));

    cout<<"---------test0 normal release end---------\n"<<endl;
}

void test1()
{
    cout<<"\n---------test1 no release begin---------"<<endl;

    shared_ptr<Person> sp_pf(new Person("zjz"));
    shared_ptr<Person> sp_ps(new Person("zcx"));

    cout<<"addr: "<<&sp_pf<<endl;
    cout<<"addr: "<<&sp_ps<<endl;

    cout<<"111 father use_count: "<<sp_pf.use_count()<<endl;
    cout<<"111 son use_count: "<<sp_ps.use_count()<<endl;

    sp_pf->setSon(sp_ps);
    sp_ps->setFather(sp_pf);

    cout<<"222 father use_count: "<<sp_pf.use_count()<<endl;
    cout<<"222 son use_count: "<<sp_ps.use_count()<<endl;

    cout<<"---------test1 no release end---------\n"<<endl;
}

void test2()
{
    cout<<"---------test2 release sequence begin---------"<<endl;

    shared_ptr<Person> sp_pf(new Person("zjz"));
    shared_ptr<Person> sp_ps(new Person("zcx"));

    cout<<"addr: "<<&sp_pf<<endl;
    cout<<"addr: "<<&sp_ps<<endl;

    cout<<"111 father use_count: "<<sp_pf.use_count()<<endl;
    cout<<"111 son use_count: "<<sp_ps.use_count()<<endl;

    sp_pf->setSon(sp_ps);
    //sp_ps->setFather(sp_pf);

    cout<<"222 father use_count: "<<sp_pf.use_count()<<endl;
    cout<<"222 son use_count: "<<sp_ps.use_count()<<endl;

    cout<<"---------test2 release sequence end---------"<<endl;
}

int main(void)
{
    test0();
    test1();
    test2();

    return 0;
}
