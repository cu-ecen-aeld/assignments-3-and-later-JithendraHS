#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading DEBUG: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

/**
 * @brief Thread function that waits, obtains a mutex, holds it for a specified time, and then releases it.
 * 
 * This function will be executed by a thread. It takes a pointer to a `thread_data` structure as its parameter.
 * The function will first wait for a specified time, then lock the mutex, hold it for another specified time,
 * and finally release the mutex. The success status of the operation is recorded in the `thread_data` structure.
 * 
 * @param thread_param Pointer to a `thread_data` structure containing the mutex and wait times.
 * @return A pointer to the `thread_data` structure passed in as `thread_param`.
 * 
 * The function returns the same `thread_data` structure passed to it. This allows the calling function to retrieve
 * the status and free the allocated memory after the thread has completed.
 */
void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    // Cast the input parameter to a pointer of type 'struct thread_data'.
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    // Store the thread's own ID in the thread_data structure for potential debugging or logging.
    thread_func_args->thread_id = pthread_self();

    // Sleep for the specified number of milliseconds before attempting to obtain the mutex.
    usleep(thread_func_args->wait_to_obtain_ms * 1000);

    // Attempt to lock the mutex. This call will block if the mutex is already locked by another thread.
    pthread_mutex_lock(thread_func_args->mutex);

    // Sleep for the specified number of milliseconds while holding the mutex.
    usleep(thread_func_args->wait_to_release_ms * 1000);

    // Unlock the mutex, allowing other threads to lock it.
    pthread_mutex_unlock(thread_func_args->mutex);

    // Indicate that the thread has successfully completed its task.
    thread_func_args->thread_complete_success = true;

    // Return the thread_data structure so that the caller can access it using pthread_join.
    return thread_param;
}

/**
 * @brief Starts a thread that waits, obtains a mutex, holds it for a specified time, and then releases it.
 * 
 * This function allocates memory for a `thread_data` structure, initializes it with the provided arguments,
 * and starts a thread using `pthread_create`. The thread will execute `threadfunc`, which waits for a specified
 * time, locks the mutex, waits again, and then unlocks the mutex. The function returns true if the thread was
 * started successfully, and false otherwise.
 * 
 * @param thread Pointer to a pthread_t where the thread ID of the created thread will be stored.
 * @param mutex Pointer to a pthread_mutex_t that the thread will lock and unlock.
 * @param wait_to_obtain_ms The number of milliseconds the thread will wait before attempting to lock the mutex.
 * @param wait_to_release_ms The number of milliseconds the thread will hold the mutex before releasing it.
 * @return true if the thread was started successfully, false if an error occurred (e.g., memory allocation failure or thread creation failure).
 */
bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    /**
     * Allocate memory for the thread_data structure.
     * This structure will hold the data to be passed to the thread function.
     */
    struct thread_data *thr_dt = (struct thread_data*)malloc(sizeof(struct thread_data));

    // Check if memory allocation was successful.
    if(thr_dt == NULL){
        printf("Failed to allocate memory !!!\n\r");
        return false;
    }

    // Initialize the fields of the thread_data structure with the provided arguments.
    thr_dt->mutex = mutex;
    thr_dt->wait_to_obtain_ms = wait_to_obtain_ms;
    thr_dt->wait_to_release_ms = wait_to_release_ms;

    /**
     * Create the thread, passing the thread_data structure as the argument.
     * The threadfunc function is used as the entry point for the new thread.
     */
    int rc = pthread_create(thread, NULL, threadfunc, (void*) thr_dt);

    // Check if the thread was created successfully.
    if(rc){
        // If thread creation failed, print an error message, free the allocated memory, and return false.
        printf("\nThread can't be created : [%s]", strerror(rc));
        free(thr_dt);
        return false;
    }

    // If thread creation was successful, return true.
    return true;
}

