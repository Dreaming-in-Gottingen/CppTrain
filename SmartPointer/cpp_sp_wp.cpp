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

    void setWpFather(weak_ptr<Person> &p) {
        wp_father = p;
    }

    void setWpSon(weak_ptr<Person> &p) {
        wp_son = p;
    }

    void printName() {
        cout<<"name: "<<name.c_str()<<endl;
    }

private:
    const string name;
    shared_ptr<Person> father;
    shared_ptr<Person> son;

    weak_ptr<Person> wp_father;
    weak_ptr<Person> wp_son;
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

void wp_getlock(weak_ptr<Person> &wp_pp)
{
    if (auto sp = wp_pp.lock()) {
        cout<<"lock ok! use_count:"<<sp.use_count()<<endl;
        sp->printName();
    } else {
        cout<<"lock fail!"<<endl;
    }
}

void test3()
{
    weak_ptr<Person> wp;
    cout<<"---------test3 weak_ptr begin---------"<<endl;
    {
        shared_ptr<Person> sp(new Person("hh"));
        wp = sp;
        wp_getlock(wp);
    }
    wp_getlock(wp);

    cout<<"---------test3 weak_ptr end---------"<<endl;
}

void test4()
{
    cout<<"\n---------test4 wp govern obj begin---------"<<endl;

    shared_ptr<Person> sp_pf(new Person("zjz"));
    shared_ptr<Person> sp_ps(new Person("zcx"));

    cout<<"sp_pf_addr: "<<&sp_pf<<", real_obj_addr: "<<sp_pf.get()<<endl;
    cout<<"sp_ps_addr: "<<&sp_ps<<", real_obj_addr: "<<sp_ps.get()<<endl;

    cout<<"111 father use_count: "<<sp_pf.use_count()<<endl;
    cout<<"111 son use_count: "<<sp_ps.use_count()<<endl;

    weak_ptr<Person> wp_pf = sp_pf;
    weak_ptr<Person> wp_ps = sp_ps;

    sp_pf->setWpSon(wp_ps);
    sp_ps->setWpFather(wp_pf);

    cout<<"222 father use_count: "<<sp_pf.use_count()<<endl;
    cout<<"222 son use_count: "<<sp_ps.use_count()<<endl;

    cout<<"---------test4 wp govern obj end---------\n"<<endl;
}


int main(void)
{
    test0();
    test1();
    test2();
    test3();
    test4();

    return 0;
}
