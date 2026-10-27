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
 // #360: a 64-bit crash (any Windows exception) leads to the 32-bit offer; quits and a closed console do not
 for(unsigned code: {0xc0000005u,0xc000001du,0xc00000fdu,0xc0000409u,0xc0000135u}) assert(ReKHCoMCrashed64(code));
 for(unsigned code: {0u,1u,2u,81u,0xc000013au}) assert(!ReKHCoMCrashed64(code));
 for(int i=0;i<3;i++) assert(ReKHCoMRuntimeMode(std::to_string(i))==i);
 assert(ReKHCoMRuntimeMode("garbage")==0);
 assert(ReKHCoMForce32("1")); assert(!ReKHCoMForce32("2"));
}
