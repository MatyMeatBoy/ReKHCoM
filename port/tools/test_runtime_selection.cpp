#include "../decompui/runtime_selection.h"
#include <cassert>
int main() {
 assert(!ReKHCoMSelect64(false,false,true,true,true));
 assert(!ReKHCoMSelect64(true,false,false,true,true));
 assert(!ReKHCoMSelect64(true,true,true,true,true));
 assert(ReKHCoMSelect64(true,false,true,true,true));
 for(unsigned code: {2u,3u,4u,0xc0000135u,0xc000007bu,0xc0000142u}) {
  assert(ReKHCoMRetry32(code,false)); assert(!ReKHCoMRetry32(code,true));
 }
 for(unsigned code: {0u,81u,1u,0xc0000005u}) assert(!ReKHCoMRetry32(code,false));
 for(int i=0;i<3;i++) assert(ReKHCoMRuntimeMode(std::to_string(i))==i);
 assert(ReKHCoMRuntimeMode("garbage")==0);
 assert(ReKHCoMForce32("1")); assert(!ReKHCoMForce32("2"));
}
