#ifndef GCOBJECT_HPP
#define GCOBJECT_HPP

#include <vector>

class GCObject {
public:
    bool marked = false;
    virtual ~GCObject();
    virtual std::vector<GCObject*> get_references() = 0;
};

#endif // !GCOBJECT_HPP
