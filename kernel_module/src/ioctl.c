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

// structure for thread list
struct thread {
    int tid;
    struct thread *next;
};

// structure for container list
struct container {
    int cid;
    struct thread *headThread;
    struct container *next;
};

struct container *containerList;



// memory allocation
void *doMalloc (size_t sz) {
    void *mem = kmalloc(sz, GFP_KERNEL);
    if (mem == NULL) {
        printk ("Out of memory! Exiting.\n");
        return NULL;
    }
    return mem;
}

// add container to the end of the container list
void addContainer (struct container *head, int new_cid) {

   struct container *new_node = doMalloc(sizeof (new_node));
   new_node->cid  = new_cid;
   new_node->headThread = NULL;
   new_node->next = NULL;
   if(head == NULL)
     {
         head=new_node;
         return;
     }
  else {
    struct thread *curr=head;
    while(curr->next!=NULL)
    {
      curr=curr->next;
    }
    curr->next=new_node;
  }

   // Copy contents of new_data to newly allocated memory.
   // Assumption: char takes 1 byte.
   // int i;
   // for (i=0; i<data_size; i++)
   //     *(char *)(new_node->data + i) = *(char *)(new_data + i);
   //
   // // Change head pointer as new node is added at the beginning
   // (*head_ref)    = new_node;

}

// add thread to the end of th thread list
void addThread (struct thread *first, int new_tid) {
    struct thread *newNode = doMalloc(sizeof (*newNode));
    newNode->tid = new_tid;
    newNode->next = NULL;
    if(first == NULL)
      {
          first=newNode;
          return;
      }
   else {
     struct thread *curr=first;
     while(curr->next!=NULL)
     {
       curr=curr->next;
     }
     curr->next=newNode;
   }

// check if container exists
struct container * getContainer(struct container *head, int new_cid)
{
  if(head == NULL)
    {
        return NULL;
    }
  return head;
  struct thread *curr=*first;
  while(curr!=NULL)
  {
    if(curr->cid == cid)
    curr=curr->next;
  }
  curr->next=newNode;
}

    // if(*curr == NULL)
    // {
    //   first
    // }
    // first->firsthreadObj = newest;
}


/**
 * Delete the task in the container.
 *
 * external functions needed:
 * mutex_lock(), mutex_unlock(), wake_up_process(),
 */
int processor_container_delete(struct processor_container_cmd __user *user_cmd)
{
    struct task_struct *task=current;
    printk("Thread id in delete  : ",(int)task->pid);
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
     struct processor_container_cmd userInfo;
     copy_from_user(&userInfo,user_cmd,sizeof(userInfo));
     int new_cid = (int)userInfo.cid;
     struct task_struct *task=current;
     int new_tid = (int)task->pid;



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
