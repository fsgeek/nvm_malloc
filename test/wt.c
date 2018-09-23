#include <stdio.h>
#include <libpmemcto.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

int main(const int argc, const char **argv, const char**envp)
{
	const char *path = "/mnt/pmem0p1/fsgeek-wt.dat";
	const char *layout = "mine";
	PMEMctopool *pcp = NULL;
	void *root = NULL;

	while (NULL == pcp) {
		pcp = pmemcto_open(path, layout);
		if (NULL != pcp) {
			break;
		}
		
		if (EINVAL == errno) {
			fprintf(stderr, "file %s is corrupt\n", path);
			unlink(path);
		}

		pcp = pmemcto_create(path, layout, 1024*1024*1024, 0600);
		if (NULL == pcp) {
			break;
		}

		root = pmemcto_get_root_pointer(pcp);
		if (NULL != root) {
			break;
		}

		root = pmemcto_malloc(pcp, 64);
		if (NULL == root) {
			fprintf(stderr, "pmemcto_malloc failed %d %s\n", errno,
				pmemcto_errormsg());
			pmemcto_close(pcp);
			unlink(path);
			pcp = NULL;
			break;
		}

		memset(root, 0, 64);
		pmemcto_set_root_pointer(pcp, root);

		// at this point we're done
		break;
	}

	if (NULL != pcp) {
		pmemcto_close(pcp);
	}

	fprintf(stderr, "Done!\n");

}
