#ifndef depthdata_INCLUDED
#define depthdata_INCLUDED

#include <memory>
class L2Data{
    private:
        int depth;
        bool writelog;
        unique_ptr<>
        
    public:
        L2Data(int depth=1, bool _log=true):writelog(_log){}
        ~L2Data();
           
        
}

#endif
