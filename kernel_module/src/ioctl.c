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

// // mutex
static DEFINE_MUTEX(lock);
// structure for thread list
struct thread {
    int tid;
    struct task_struct *current_thread;
    struct thread *next;
};
// structure for container list
struct container {
    int cid;
    struct thread *headThread;
    struct container *next;
};

struct container *containerList =  NULL;

// // memory allocation
// void *doMalloc (size_t sz) {
//     void *mem = kmalloc(sz, GFP_KERNEL);
//     if (mem == NULL) {
//         printk ("No memory to allocate\n");
//         return NULL;
//     }
//     return mem;
// }

///////////////////////////////////////////////////////////////////////////////////////////////////
// add thread to the end of th thread list
void addThread (int curr_cid, int new_tid) {
    struct thread *curr=NULL;
    struct thread *first = NULL;
    // struct thread *newNode = NULL;
    struct container * currContainer = containerList;
    while(currContainer != NULL)
    {
      if(currContainer->cid == curr_cid)
      {
        first = currContainer->headThread;
        break;
      }
      currContainer = currContainer->next;
    }
    printk("\t\tstart addThread\n");
    struct thread* newNode = (struct thread*) kcalloc(1, sizeof(struct thread), GFP_KERNEL);
    // newNode = doMalloc(sizeof (struct thread));
    newNode->current_thread = current;
    newNode->tid = new_tid;
    newNode->next = NULL;
    if(first == NULL)
      {
          printk("first is null\n");
          first=newNode;
          // newNode=NULL;
          mutex_unlock(&lock);
          return;
      }
   else {
     curr=first;
     while(curr->next!=NULL)
     {
       if(curr->tid == new_tid)
       {
         printk("Thread already exists in container\n");
         kfree(newNode);
         newNode = NULL;
         mutex_unlock(&lock);
         return;
       }
       curr=curr->next;
     }
     curr->next=newNode;
   }
   // newNode=NULL;
   mutex_unlock(&lock);
   // sleep
   set_current_state(TASK_INTERRUPTIBLE);
   schedule();
   printk("\t\tend addThread\n");
}

// remove thread from given container
void removeTopThread (int curr_cid) {
    struct container *curr = NULL;
    struct container *prev = NULL;
    printk("\t\tstart removeTopThread\n");
    curr = containerList;
    while(curr!=NULL)
    {
      printk("in while loop\n");
      if(curr->cid == curr_cid)
      {
        printk("in if curr->cid == curr_cid\n");
        struct thread *first = curr->headThread;
        printk("variable->'first' assigned \n");
        if(first == NULL)
        {
          printk("container is empty!!\n");
          if(prev == NULL) containerList = curr->next;
          else prev->next = curr->next;
          printk("free container mem\n");
          kfree(curr);
          curr=NULL;
          mutex_unlock(&lock);
          return;
        }
        if(first->next == NULL)
        {
          printk("only thread in container list\n");
          printk("free thread mem\n");
          kfree(first);
          first=NULL;
          if(prev == NULL) containerList = curr->next;
          else prev->next = curr->next;
          printk("free container mem\n");
          kfree(curr);
          curr=NULL;
          mutex_unlock(&lock);
          return;
        }
        curr->headThread = first->next;
        printk("wake up next process\n");
        wake_up_process(curr->headThread->current_thread);
        kfree(first);
        first=NULL;
        return;
      }
      prev = curr;
      curr = curr->next;
    }
    printk("Container with CID: %d not found\n", curr_cid);
    printk("\t\tend removeTopThread\n");
    return;
}

// add container to the end of the container list
void addContainer (int new_cid, int new_tid) {
   struct container *curr=NULL;
   // struct container *new_node = NULL;
   printk("\t\tstart addContainer\n");
   struct container* new_node = (struct container*) kcalloc(1, sizeof(struct container), GFP_KERNEL);
   // new_node = doMalloc(sizeof (struct container));
   new_node->cid  = new_cid;
   new_node->headThread = NULL;
   new_node->next = NULL;
   if(containerList == NULL)
     {
         printk("head is null\n");
         containerList=new_node;
         containerList->headThread = (struct thread*) kcalloc(1, sizeof(struct thread), GFP_KERNEL);
         // head->headThread = doMalloc(sizeof (struct thread));
         containerList->headThread->current_thread = current;
         containerList->headThread->tid = new_tid;
         containerList->headThread->next = NULL;
         mutex_unlock(&lock);
         return;
     }
  else {
    curr = containerList;
    while(curr->next!=NULL)
    {
      curr = curr->next;
    }
    curr->next=new_node;
  }
  printk("\t\tend addContainer\n");
  addThread(curr->next->cid, new_tid);
}


// check if container exists or add it if it does not
void addToContainer(int new_cid, int new_tid)
{

  struct container *curr=containerList;
  printk("\t\tstart addTOContainer\n");
  printk("Add to container:\tCID: %d\tTID: %d\n",new_cid,new_tid);
  if(containerList == NULL)
    {
        printk("head is null in addTOcontainer\n");
        addContainer(new_cid, new_tid);
        return;
    }
  while(curr!=NULL)
  {
    if(curr->cid == new_cid)
    {
      addThread(curr->cid, new_tid);
      return;
    }
    curr=curr->next;
  }
  addContainer(new_cid, new_tid);
  printk("\t\tend addTOContainer\n");
  return;
}

// print the map
void printMap(struct container *head)
{
  printk("\n-----------------------------------------------------------------------------------------------------\n");
  mutex_lock(&lock);
  if(head == NULL)
  {
    printk("\nNo containers\n");
    mutex_unlock(&lock);
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
  mutex_unlock(&lock);
  printk("\n----------------------------------------------------------------------------------------------------------------\n");
}

// remove current thread and add it to the end of queue
void addCurrentThreadAtEnd (int cid) {
    struct container *currContainer=NULL;
    struct thread *first=NULL;
    printk("\t\tstart addCurrentThreadAtEnd\n");
    currContainer=containerList;
    printk("add current thread at end:\tCID: %d\n",cid);
    //find the container for cid
    while(currContainer!=NULL)
    {
       if(currContainer->cid==cid)
       {
         first=currContainer->headThread;
         break;
       }
       currContainer=currContainer->next;
    }
    if(currContainer == NULL)
    {
      printk("container not found\n");
      mutex_unlock(&lock);
      return;
    }
    printk("found curr container\n");
    //first is the head to thread list
    if(first == NULL || first->next==NULL)
      {
          printk("Nothing to do \n");
          mutex_unlock(&lock);
          return;
      }
   else {
     struct thread *newFirst=first->next;
     struct thread *curr=first;
     while(curr->next!=NULL)
     {
       curr=curr->next;
     }
     curr->next=first;
     first->next=NULL;

     //tempHead is a pointer to current container
     currContainer->headThread=newFirst;
     wake_up_process(newFirst->current_thread);
     mutex_unlock(&lock);
     set_current_state(TASK_INTERRUPTIBLE);
     schedule();
     printk("\nstopping curr thread and starting next one\n");
     printk("\t\tend addCurrentThreadAtEnd\n");
     return;
   }
}

int findContainerForThread(int curr_tid)
{
  struct container *curr_container=NULL;
  printk("\t\tstart findContainerForThread\n");
  curr_container = containerList;
  printk("find container for thread:\tTID: %d\n",curr_tid);
  while(curr_container != NULL)
  {
    if(curr_container->headThread!=NULL)
    {
      if(curr_container->headThread->tid == curr_tid)
      {
        printk("\t\tending findContainerFor Thread with container found\n");
        return curr_container->cid;
      }
    }
    curr_container = curr_container->next;
  }
  printk("\t\tend findContainerForThread\n");
  return -1;
}

/**
 * Delete the task in the container.
 *
 * external functions needed:
 * mutex_lock(), mutex_unlock(), wake_up_process(),
 */
int processor_container_delete(struct processor_container_cmd __user *user_cmd)
{
    int curr_cid;
    struct processor_container_cmd* userInfo = (struct processor_container_cmd*) kcalloc(1, sizeof(struct processor_container_cmd), GFP_KERNEL);
    mutex_lock(&lock);
    printk("\t\tprocessor_container_delete ****************************\n");
    copy_from_user(userInfo, user_cmd, sizeof(struct processor_container_cmd));
    curr_cid = (int)userInfo->cid;
    kfree(userInfo);
    userInfo = NULL;
    printk("deleting container CID:\t%d\n", curr_cid);
    removeTopThread(curr_cid);
    // printMap(containerList);
    mutex_unlock(&lock);
    printk("\t\tprocessor_container_delete ****************************\n");
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
     struct task_struct *task=NULL;

     struct processor_container_cmd* userInfo = (struct processor_container_cmd*) kcalloc(1, sizeof(struct processor_container_cmd), GFP_KERNEL);
     mutex_lock(&lock);
     task=current;
     printk("\t\tprocessor_container_create ****************************\n");
     copy_from_user(userInfo,user_cmd,sizeof(struct processor_container_cmd));
     new_cid = (int)userInfo->cid;
     new_tid = (int)task->pid;
     kfree(userInfo);
     userInfo = NULL;
     printk("creating CID: %d\tTIP: %d\n",new_cid,new_tid);
     addToContainer(new_cid, new_tid);
     // above function unlocks mutex
     // printMap(containerList);
     printk("\t\tprocessor_container_create ****************************\n");
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
    int curr_cid=NULL;
    int curr_tid=NULL;
    struct task_struct *task=NULL;
    mutex_lock(&lock);
    task=current;
    printk("\t\tprocessor_container_switch ****************************\n");
    curr_tid = (int)task->pid;
      curr_cid = findContainerForThread(curr_tid);
     if(curr_cid!=-1)
        addCurrentThreadAtEnd(curr_cid);
        // above function unlocks mutex
     else
     {
        printk("\ncontainer not found for swapping\n");
        mutex_unlock(&lock);
     }
     // printMap(containerList);
     printk("\t\tprocessor_container_switch ****************************\n");
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
    if(containerList == NULL)
    {
      printk("\nContainer List is NULL!!!!!!!!!!!!!!!!!!!!!!\n");
    }
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
