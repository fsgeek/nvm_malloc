#include "common.h"

namespace nvb {

#if defined(USE_MALLOC) || defined(USE_PMDK)
object_table_t _object_table;
#endif

#if defined(USE_PMDK)
PMEMctopool *pcp;
#endif

}
