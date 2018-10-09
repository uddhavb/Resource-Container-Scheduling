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

///////////////////////////////////////////////////////////////////////////////////////////////////
// add thread to the end of th thread list
struct thread *addThread (struct thread *first, int new_tid) {
    struct thread *curr=NULL;
    struct thread *newNode = NULL;
    printk("\t\tstart addThread\n");
    mutex_lock(&lock);
    newNode = doMalloc(sizeof (*newNode));
    newNode->current_thread = current;
    newNode->tid = new_tid;
    newNode->next = NULL;
    if(first == NULL)
      {
          printk("first is null\n");
          first=newNode;
          // newNode=NULL;
          mutex_unlock(&lock);
          return first;
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
         return first;
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
   return first;
}

// remove thread from given container
struct container *removeTopThread (struct container *head, int curr_cid) {
    struct container *curr = NULL;
    struct container *prev = NULL;
    printk("\t\tstart removeTopThread\n");
    mutex_lock(&lock);
    curr = head;
    while(curr!=NULL)
    {
      if(curr->cid == curr_cid)
      {
        struct thread *first = curr->headThread;
        if(first->next == NULL)
        {
          kfree(first);
          first=NULL;
          if(prev == NULL) prev = curr->next;
          else prev->next = curr->next;
          kfree(curr);
          curr=NULL;
          mutex_unlock(&lock);
          return prev;
        }
        curr->headThread = first->next;
        wake_up_process(first->next->current_thread);
        kfree(first);
        first=NULL;
        mutex_unlock(&lock);
        return head;
      }
      prev = curr;
      curr = curr->next;
    }
    printk("Container with CID: %d not found\n", curr_cid);
    printk("\t\tend removeTopThread\n");
    mutex_unlock(&lock);
    return head;
}

// add container to the end of the container list
struct container *addContainer (struct container *head, int new_cid) {
   struct container *curr=NULL;
   struct container *new_node = NULL;
   printk("\t\tstart addContainer\n");
   mutex_lock(&lock);
   new_node = doMalloc(sizeof (new_node));
   new_node->cid  = new_cid;
   new_node->headThread = NULL;
   new_node->next = NULL;
   if(head == NULL)
     {
         printk("head is null\n");
         head=new_node;
         // new_node=NULL;
         mutex_unlock(&lock);
         return head;
     }
  else {
    curr = head;
    while(curr->next!=NULL)
    {
      curr = curr->next;
    }
    curr->next=new_node;
  }
  // new_node=NULL;
  printk("\t\tend addContainer\n");
  mutex_unlock(&lock);
  return curr->next;
}


// check if container exists or add it if it does not
struct container *addToContainer(struct container *head, int new_cid, int new_tid)
{
  struct container * curr_container;
  struct container *curr=head;
  printk("\t\tstart addTOContainer\n");
  printk("Add to container:\tCID: %d\tTID: %d\n",new_cid,new_tid);
  if(head == NULL)
    {
        printk("head is null in addTOcontainer\n");
        head = addContainer(head, new_cid);
        curr_container = head;
        curr_container->headThread = addThread(curr_container->headThread, new_tid);
        return head;

    }
  while(curr!=NULL)
  {
    if(curr->cid == new_cid)
    {
      curr_container->headThread = addThread(curr->headThread, new_tid);
      return head;
    }
    curr=curr->next;
  }
  curr_container = addContainer(head, new_cid);
  curr_container->headThread = addThread(curr_container->headThread, new_tid);
  printk("\t\tend addTOContainer\n");
  return head;
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
struct container *addCurrentThreadAtEnd (struct container *head,int cid) {
    struct container *currContainer=NULL;
    struct thread *first=NULL;
    printk("\t\tstart addCurrentThreadAtEnd\n");
    mutex_lock(&lock);
    currContainer=head;
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
    printk("found curr conatiner\n");
    //first is the head to thread list
    if(first == NULL || first->next==NULL)
      {
          printk("Nothing to do \n");
          mutex_unlock(&lock);
          return head;
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
     return head;
   }
}

int findContainerForThread(struct container *head, int curr_tid)
{
  struct container *curr_container=NULL;
  printk("\t\tstart findContainerForThread\n");
  mutex_lock(&lock);
  curr_container = head;
  printk("find container for thread:\tTID: %d\n",curr_tid);
  while(curr_container != NULL)
  {
    if(curr_container->headThread!=NULL)
    {
      if(curr_container->headThread->tid == curr_tid)
      {
        mutex_unlock(&lock);
        printk("\t\tending findContainerFor Thread with container found\n");
        return curr_container->cid;
      }
    }
    curr_container = curr_container->next;
  }
  mutex_unlock(&lock);
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
    struct processor_container_cmd userInfo;
    printk("\t\tprocessor_container_delete ****************************\n");
    copy_from_user(&userInfo,user_cmd,sizeof(userInfo));
    curr_cid = (int)userInfo.cid;
    printk("deleting container CID:\t%d\n", curr_cid);
    containerList = removeTopThread(containerList,curr_cid);
    printMap(containerList);
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
     struct processor_container_cmd userInfo;
     struct task_struct *task=current;
     printk("\t\tprocessor_container_create ****************************\n");
     copy_from_user(&userInfo,user_cmd,sizeof(userInfo));
     new_cid = (int)userInfo.cid;
     new_tid = (int)task->pid;
     printk("creating CID: %d\tTIP: %d\n",new_cid,new_tid);
     containerList = addToContainer(containerList, new_cid, new_tid);
     printMap(containerList);
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
    struct task_struct *task=current;
    printk("\t\tprocessor_container_switch ****************************\n");
    curr_tid = (int)task->pid;
      curr_cid = findContainerForThread(containerList, curr_tid);
     if(curr_cid!=-1)
        containerList = addCurrentThreadAtEnd(containerList, curr_cid);
     else printk("\ncontainer not found for swapping\n");
     printMap(containerList);
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