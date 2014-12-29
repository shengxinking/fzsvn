/*
 *
 *
 *
 */

#ifndef __TIME_HPP__
#define __TIME_HPP__

class Time {

    public:
    Time(int ss = 0, int mm = 0, int hh = 0);
    ~Time();

    static void set_default(int ss = 0, int mm = 0, int hh = 0);

    int second() const;
    int minute() const;
    int hour() const;
    
    int add_second();
    int add_minute();
    int add_hour();


    private:
    int          s;
    int          m;
    int          h;
    struct       Cache;
    Cache        cache;
    Time         default_time;
};



#endif
