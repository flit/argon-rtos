@ingroup ar_thread

### Priorities

Thread priorities range from 0 to 255. Higher values are higher priorities, with 255 being the
highest priority. Priority 0 is reserved for the idle thread. The highest priority
thread that is ready to run will always get the processor. That means that if there is only one
high priority thread, it can starve lower priority threads if it never relinquishes control by
sleeping or blocking on a resource. Threads with the same priority will preempt each other in a
round robin order every system tick.

### Encapsulated thread classes

If you want to fully encapsulate a thread you can create a subclass of Thread that provides its
own init() method which calls the original Thread::init(). You can either pass a pointer to a
static function to the base init() method, as usual, or you can override the virtual
Thread::threadEntry() method. In the latter case, you can simply pass NULL for the entry point to
the base init() method. To pass values to the thread function, simply create member variables and
set them in your subclass' init() method.

Here's an example subclass that uses a member function as the entry point:
~~~~~{.cpp}
 class MySubclassThread : public Ar::Thread
 {
 public:
     ar_status_t init()
     {
         // Pass NULL for the entry point. It's not needed because you are
         // overriding threadEntry() below.
         return Thread::init("my thread", NULL, this, m_stack, sizeof(m_stack), 32);
     }

 protected:
     // Static memory for the stack.
     uint8_t m_stack[4096];

     // Override the default Thread implementation.
     virtual void threadEntry()
     {
         // Implement your thread here.
     }

 };
~~~~~



