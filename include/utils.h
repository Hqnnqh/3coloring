#pragma once

#define SHM_NAME "/shared_memory_3coloring"
#define SHM_FLAGS O_CREAT | O_RDWR

#define SEM_FREE "/sem_free_3coloring"
#define SEM_USED "/sem_used_3coloring"
#define SEM_MUTEX "/sem_mutex_3coloring"

#define PERM 0600
