//////////////////////////////////////////////////////////////////////
//                      North Carolina State University
//
//
//
//                             Copyright 2016
//
////////////////////////////////////////////////////////////////////////
//
// This program is free software; you can redistribute it and/or modify it
// under the terms and conditions of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
//
////////////////////////////////////////////////////////////////////////
//
//   Author:  Hung-Wei Tseng, Yu-Chia Liu
//
//   Description:
//     Core of Kernel Module for Processor Container
//
////////////////////////////////////////////////////////////////////////

#include "processor_container.h"

#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/kthread.h>

// structure for container list
typedef struct container {
    int cid;
    threadObj *firsthreadObj;
    struct container *next;
} containerObj;

// structure for thread list
typedef struct thread {
    int tid;
    struct thread *next;
} threadObj;

// memory allocation
void *doMalloc (size_t sz) {
    void *mem = kmalloc(sz, GFP_KERNEL);
    if (mem == NULL) {
        printk ("Out of memory! Exiting.\n");
        return NULL;
    }
    return mem;
}

// add container to the start of the container list
void addContainer (containerObj **first, char *new_cid) {

    containerObj *newest = doMalloc (sizeof (*newest));
    newest->cid = *new_cid;
    newest->next = *first;
    *first = newest;
}

// add thread to the end of th thread list
void addThread (containerObj *first, char *new_tid) {
    threadObj *newest = doMalloc (sizeof (*newest));
    newest->tid = *new_tid;
    newest->next = first->firsthreadObj;  
    first->firsthreadObj = newest;
}


/**
 * Delete the task in the container.
 *
 * external functions needed:
 * mutex_lock(), mutex_unlock(), wake_up_process(),
 */
int processor_container_delete(struct processor_container_cmd __user *user_cmd)
{
    printk("delete\n");
    // printk("user cmd:\t %llu\t%llu", user_cmd->cid, user_cmd->op);
    return 0;
}

/**
 * Create a task in the corresponding container.
 * external functions needed:
 * copy_from_user(), mutex_lock(), mutex_unlock(), set_current_state(), schedule()
 *
 * external variables needed:
 * struct task_struct* current
 */
int processor_container_create(struct processor_container_cmd __user *user_cmd)
{
  printk("create\n");
  // printk("user cmd:\t %llu\t%llu", user_cmd->cid, user_cmd->op);
  // create a new container
    return 0;
}

/**
 * switch to the next task within the same container
 *
 * external functions needed:
 * mutex_lock(), mutex_unlock(), wake_up_process(), set_current_state(), schedule()
 */
int processor_container_switch(struct processor_container_cmd __user *user_cmd)
{
    printk("switch thread\n");
    return 0;
}

/**
 * control function that receive the command in user space and pass arguments to
 * corresponding functions.
 */
int processor_container_ioctl(struct file *filp, unsigned int cmd,
                              unsigned long arg)
{
    printk("in ioctl\n");
    switch (cmd)
    {
    case PCONTAINER_IOCTL_CSWITCH:
        return processor_container_switch((void __user *)arg);
    case PCONTAINER_IOCTL_CREATE:
        return processor_container_create((void __user *)arg);
    case PCONTAINER_IOCTL_DELETE:
        return processor_container_delete((void __user *)arg);
    default:
        return -ENOTTY;
    }
}
