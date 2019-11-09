#ifndef _NON_COPYABLE_H
#define _NON_COPYABLE_H

namespace svrlib {

class noncopyable {
protected:
    noncopyable(){}
    virtual ~noncopyable(){}
private:
    noncopyable(const noncopyable&);
    noncopyable& operator = (const noncopyable&);
};//end class noncopyable

}

#endif
 
