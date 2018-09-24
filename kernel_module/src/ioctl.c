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
        printk ("No memory to allocate\n");
        return NULL;
    }
    return mem;
}

// add container to the end of the container list
struct container *addContainer (struct container *head, int new_cid) {
   struct container *new_node = doMalloc(sizeof (new_node));
   new_node->cid  = new_cid;
   new_node->headThread = NULL;
   new_node->next = NULL;
   if(head == NULL)
     {
         head=new_node;
     }
  else {
    struct container *curr = head;
    while(curr->next!=NULL)
    {
      curr = curr->next;
    }
    curr->next=new_node;
  }
  return head;
}

// add thread to the end of th thread list
struct thread *addThread (struct thread *first, int new_tid) {
    struct thread *newNode = doMalloc(sizeof (*newNode));
    newNode->tid = new_tid;
    newNode->next = NULL;
    if(first == NULL)
      {
          first=newNode;
          return first;
      }
   else {
     struct thread *curr=first;
     while(curr->next!=NULL)
     {
       curr=curr->next;
     }
     curr->next=newNode;
   }
   return first;
}

// check if container exists or add it if it does not
struct container *addToContainer(struct container *head, int new_cid, int new_tid)
{
  struct container * curr_container;
  struct container *curr=head;
  if(head == NULL)
    {
        curr_container = addContainer(head, new_cid);
        curr_container->headThread = addThread(curr_container->headThread, new_tid);
        return curr_container;

    }
  while(curr!=NULL)
  {
    if(curr->cid == new_cid)
    {
      curr_container->headThread = addThread(curr->headThread, new_tid);
      return curr;
    }
    curr=curr->next;
  }
  curr_container = addContainer(head, new_cid);
  curr_container->headThread = addThread(curr_container->headThread, new_tid);
  return curr_container;
}

// print the map
void printMap(struct container *head)
{
  if(head == NULL)
  {
    printk("\nNo containers\n");
    return;
  }
  else
  {
    struct thread *currThread;
    struct container *currContainer = head;
    while(currContainer != NULL)
    {
      printk("\nCID:\t%d\t",currContainer->cid);
      currThread = currContainer->headThread;
      while(currThread != NULL)
      {
        printk("TID:\t%d\t",currThread->tid);
        currThread = currThread->next;
      }
      printk("\n");
      currContainer = currContainer->next;
    }
  }
}


/**
 * Delete the task in the container.
 *
 * external functions needed:
 * mutex_lock(), mutex_unlock(), wake_up_process(),
 */
int processor_container_delete(struct processor_container_cmd __user *user_cmd)
{
    // struct task_struct *task=current;
    // printk("Thread id in delete  : ",(int)task->pid);
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
     int new_cid;
     int new_tid;
     struct processor_container_cmd userInfo;
     struct task_struct *task=current;
     copy_from_user(&userInfo,user_cmd,sizeof(userInfo));
     new_cid = (int)userInfo.cid;
     new_tid = (int)task->pid;
     // printk("CID: %d\tTIP: %d\n",new_cid,new_tid);
     containerList = addToContainer(containerList, new_cid, new_tid);
     printMap(containerList);
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
    // printk("switch thread\n");
    return 0;
}

/**
 * control function that receive the command in user space and pass arguments to
 * corresponding functions.
 */
int processor_container_ioctl(struct file *filp, unsigned int cmd,
                              unsigned long arg)
{
    // printk("in ioctl\n");
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
