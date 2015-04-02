#include <deque>
#include <string>
using namespace std;
struct data{
    virtual ~data(){}
};

struct ddd{
    int x;
    double d;
    string str;
};


int main(){
    deque<ddd> x;
    x.push_back({0, 0.0, ""});
}
