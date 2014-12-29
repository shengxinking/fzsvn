/*
 *  resource alloc and free.
 *
 *  write by Forrest.zhang
 */

#ifndef __FILEPTR_HPP__
#define __FILEPTR_HPP__

#include <string>
#include <iostream>
#include <cstdio>

using namespace std;

class FilePtr {

public:
    // member class
    class        FilePtrErr;

    // constructor
    FilePtr(const string& file, const string& mode);

    // destructor
    ~FilePtr();

    // members functions
    operator FILE*() const;
    FilePtr* operator -> ();

private:
    FILE*          fp;

    // prohibit copy constructor and assign
    FilePtr(const FilePtr& fp);
    FilePtr& operator = (const FilePtr& fp);
};

class FilePtr::FilePtrErr {

public:
    // constructor
    FilePtrErr(const string& msg);
    FilePtrErr(const FilePtrErr& e);

    // destructor
    ~FilePtrErr();

    // member functions
    void print_debug();

private:
    string        msg;
};

inline FilePtr::FilePtrErr::FilePtrErr(const string& _msg)
    :msg(_msg)
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline FilePtr::FilePtrErr::FilePtrErr(const FilePtrErr& e)
    :msg(e.msg)
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline FilePtr::FilePtrErr::~FilePtrErr()
{
    cout << __PRETTY_FUNCTION__ << endl;
}

inline void FilePtr::FilePtrErr::print_debug()
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << msg << endl;
}



inline FilePtr::FilePtr(const string& file, const string& mode)
    try
{
    cout << __PRETTY_FUNCTION__ << endl;
    fp = fopen(file.c_str(), mode.c_str());
    if (!fp)
	throw FilePtrErr("open file error");
}
catch (...) {
    cout << "a error in FilePtr constructor occured" << endl;
}

inline FilePtr::~FilePtr()
{
    cout << __PRETTY_FUNCTION__ << endl;

    if (uncaught_exception())
	cout << "a exception is occured but not caught" << endl;

    fclose(fp);
}

inline FilePtr::operator FILE* () const
{
    cout << __PRETTY_FUNCTION__ << endl;
    return fp;
}

inline FilePtr* FilePtr::operator ->()
{
    cout << __PRETTY_FUNCTION__ << endl;
    return this;
}

#endif
