## سیستم‌های عامل - تمرین گروهی دوم

### مشخصات گروه

>> نام، نام خانوادگی و ایمیل خود را در ادامه وارد کنید.

رضا یاربخش <ryarbakhsh@gmail.com>

نام نام خانوادگی <email@domain.example>

نام نام خانوادگی <email@domain.example>

نام نام خانوادگی <email@domain.example>

# project2

# ماموریت 1: ساعت زنگ‌دار بهینه
## 1. داده ساختارها و توابع

در `thread.c` موارد زیر را اضافه میکنیم:

- struct list_elem sleep_element`:

جهت افزودن به لیست

- `int64_t wake_time`:

زمان بیدار شدن ریسه

و در `timer.c` لیست زیر را اضاف میکنیم:

- `static struct list sleep_list`:

لیست ریسه های خواب

- `static struct lock sleep_lock`:

قفل برای عملیات خواب

## 2. الگوریتم‌ها
- هر دو تابع تحت قفل `sleep_lock` اجرا میشوند. در تابع `timer_sleep` ابتدا زمان بیدار شدن را محاسبه و ذخیره میکنیم و سپس آن را به لیست ریسه های در حال خواب اضافه کرده و در حالت `blocking` قرار میدهیم.در تابع `timer interrupt handler` به ترتیب ریسه های موجود در `sleep_list` را چک کرده و تا جایی که نیاز باشد آن ها را بیدار میکنیم (`sleep_list` یک لیست مرتب است)
- از آنجا که `sleep_list` یک لیست مرتب است با o(n) میتوان تمام ریسه های مورد نیاز را بیدار کرد.
## 3. به‌هنگام‌سازی
- با استفاده از `sleep_lock` هیچ دو ریسه همزمان نمیتوانند در `timer_sleep` عمل کنند.
- به دلیل وجود `sleep_lock` دو ریسه همزمان نمیتوانند در `interrupt handler` و `timer_sleep` باشند و بنابراین `race condition` به وجود نمی‌آید.
## 4. منطق
- طراحی فعلی با استفاده از نگهداری لیست مرتب شده ریسه‌های خواب نسبت به لیست اصلی زمان صرف شده در `interrupt handler` را کاهش و زمان `timer_sleep` را افزایش میدهد. با توجه به این که `interrupt handler` بسیار بیشتر صدا زده میشود اینکار بسیار سودمند است. همچنین به جای غیرفعال کردن `interrupt` ها از `lock` برای جلوگیری از `race` استفاده شده است که بسیار بهتر است.
# ماموریت 2: زمان‌بند اولویت‌دار

اضافه کردن effective_priority به ترد
تغییر scheduler
تغییر sema up
تغییر lock acquire 
توی لاک ریلیز priority رو برگردونیم
توی set_priority هم اگه لازم شد yield کنیم
دو تا نکته یکی انتراپت و یکی barrier ها
توی cond signal هم باید به اولویت‌ترین رو بدیم

## 1. داده ساختارها و توابع

تغییرات زیر را در داده‌ساختارها می‌دهیم:


    /* thread.h */
    struct thread
    {
      //...
      int eff_priority;
      struct lock *waiting_lock; //The lock the thread is waiting for, if any.
      struct list acquired_locks; //List of this thread acquired locks
      //...
    };
    
    /* synch.h */
    struct lock
    {
      //...
      struct list_elem elem; //For storing in acquired_locks;
      //...
    };

سپس توابع زیر را در `thread.c` تغییر می‌دهیم:

    void init_thread (void); //Initializing thread new values
    static struct thread* next_thread_to_run (void); //Choose thread with max effec_priority
    void thread_set_priority (int new_priority); //Calls set_eff_priority if needed
    /* Added */
    void thread_set_eff_priority(int new_eff_priority); //Yield if priority changes

و در فایل `synch.c` توابع زیر تغییر می‌کنند:

    void sema_up (struct semaphore *sema); //Choose waiting thread with max effec_priority
    void lock_acquire (struct lock *lock); //Update lock holder's chain threads effective priority 
    void lock_release (struct lock *lock); //Set back effective priority by calling thread_set_eff_priority
    void cond_signal (struct condition *cond, struct lock *lock UNUSED); //Send signalto max effec_priority thread
    void cond_wait (struct condition *cond, struct lock *lock); //Might change due to adding barriers to prevent optimization



## 2. الگوریتم‌ها

ما برای هر ترد مقدار `eff_priority` را نگه می‌داریم که در صورت donation توسط waiterهای لاک‌های acquire شده‌ی آن (مستقیم و غیر مستقیم) اپدیت می‌شود و به روز می‌شود. 

سپس در ۳ حالت زیر:

- انتخاب ترد بعدی در `next_thread_to_run` 
- انتخاب کسی که سمافور را down می‌کند (در `sema_up`)
- انتخابی کسی که در متغیر شرطی به آن سیگنال می‌فرستیم بین waiterها.

روی کاندیدهای ممکن در لیستشان حرکت می‌کنیم و تردی که بیشترین `eff_priority` را دارد را انتخاب می‌کنیم.  مثلا در در `next_thread_to_run` بین تردهای در صف `ready` آن ترد که بیشترین `eff_priority` را دارد انتخاب می‌کنیم.

برای مشخص کردن و به روز کردن `eff_priority` تنها دو تابع `lock_acquire` و `lock_release` تغییر می‌کنند.

روش کار اینطور است که در `lock_acquire` برای یک ترد، قبل از sema_down ، **به شکل اینتراپت خاموش** کار زیر را می‌کنیم:

- بررسی می‌کنیم اگر توانستیم در لحظه لاک را بگیریم (holder نداشت) آن را میگیریم و تمام. (اینتراپت را به حالت قبل بر میگردانیم). اگر با احتمالی در این لحظه کس دیگری داشت روی این لاک wait می‌کرد چون این ترد ران شده‌است یعنی بین بقیه از اولویت موثر بیشتری برخوردار بوده‌است.
- اگر نتوانستیم (یعنی باید wait کنیم، سپس) مقدار waiting_lock این ترد را برابر با این lock قرار می‌دهیم. روی holder لاک می‌رویم و eff_priority آن را با توجه به eff_priority خودمان به روز می‌کنیم، سپس اگر holder دارد روی لاکی wait می‌کند روی آن می‌رویم و همین روند را ادامه می‌دهیم تا تردی روی چیزی wait نکند.  به این شکلی مقدار تمام تردهایی که به نوعی ما می‌توانیم donate شان کنیم به روز می‌شود. سپس اینتراپت‌ها را به حالت عادی بر میگردانیم.  `sema_down` می‌کنیم تا نوبتمان شود. 

در `lock_release` باید `eff_priority` ترد را بازگردانیم. این مقدار لزوما برابر با `priority` نیست. زیرا ممکن است لاک‌های دیگری باشند که donation دارند. بنابراین روی لیست لاک‌هایی که `acquire` کردیمشان فور می‌زنیم و سپس روی waitingهای هر کدام می‌رویم و ماکسیمم مقدار `eff_priority` آن‌ها و `priority` ترد را به عنوان مقدار جدید `eff_priority` می‌گذاریم (با فراخوانی تابع thread_set_eff_priority، در این تابع اگر کمتر شود مقدار thread yield می‌شود). در این بخش، به این دلیل که ممکن نیست scheduler به ترد ویتینگ برود حس کنیم جلوی interrupt ها نیازی نیست بگیریم. اما غلط است، چون ممکن‌ است تردهای جدید wait کنند روی لاک و حرکت روی لیست را دچار مشکل کنند. بنابراین این بخش باید در حالت غیرفعال بودن interrupt هندل شود. (همه‌ی این کارها قبل از `sema_up` انجام می‌شود)

این روش برای ریلیز شامل donation های غیر مستقیم نیز می‌شود. زیرا waiterهای لاک‌های دیگر در `eff_priority` شان `priority` تردهایی که به آن‌ها wait می‌کنند لحاظ شده است.

مثلا حالت زیر را در نظر بگیرید. 
thread A p:30 has lock 1 and lock 2
thread B p:40 has lock 3 and is waiting for lock 1
thread C p:60 is waiting for lock 2
thread D p:50 is waiting for lock 3
حال در این لحظه `eff_priority` ترد A برابر با ۶۰ است. اما هنگامی که لاک دوم رها شود. مقدار آن برابر با اولویت موثر ترد B می‌شود که 50 است زیرا ترد D هنگام تلاش برای `acquire` کردن مقدار `eff_priority` ترد B را برابر با 50 کرده‌است.

در `thread_set_priority` نیز اولویت پایه ترد تغییر میکند و اگر از  `eff_priority` بیشتر شود تابع `thread_set_eff_priority` را صدا می‌زند. 

تابع `thread_set_eff_priority` توسط `lock_release` و `thread_set_priority` صدا زده می‌شود و مقدار `eff_priority` را تغییر میدهد که اگر این مقدار کم شده باشد yield میکنن که scheduler به ترد مناسب بدهد سیستم را.


## 3. به‌هنگام‌سازی
- در فایل thread.c متغیرهای global شاملِ
    
    /* List of processes in THREAD_READY state, that is, processes
       that are ready to run but not actually running. */
    static struct list ready_list;
    
    /* List of all processes.  Processes are added to this list
       when they are first scheduled and removed when they exit. */
    static struct list all_list;
    
    /* Idle thread. */
    static struct thread *idle_thread;
    
    /* Initial thread, the thread running init.c:main(). */
    static struct thread *initial_thread;
    
    /* Lock used by allocate_tid(). */
    static struct lock tid_lock;
    
    /* Statistics. */
    static long long idle_ticks;    /* # of timer ticks spent idle. */
    static long long kernel_ticks;  /* # of timer ticks in kernel threads. */
    static long long user_ticks;    /* # of timer ticks in user programs. */
    
    /* Scheduling. */
    #define TIME_SLICE 4            /* # of timer ticks to give each thread. */
    static unsigned thread_ticks;   /* # of timer ticks since last yield. */
    

می‌شود. تمام تغییراتی که روی لیست‌ها انجام شده، یا پس از خاموش کردن interruptها بوده یا در تابعی بوده که در آن خاموش بودن interruptها assert شده است. تغییرات روی idle_thread و initial_thread هم پس از خاموش کردن interruptها صورت گرفته است. تغییرات روی متغیرهای مربوط به statistics هم زمانی اتفاق می‌افتد که در تابع thread_tick هستیم. این تابع زمانی صدا زده می‌شود که در interrupt_context هستیم. تغییر دیگری روی thread_ticks در thread_schedule_tail هم داریم که در آن تابع، خاموش بودن interruptها assert شده است. بنابراین می‌توان نتیجه گرفت که در فایل thread.c شرایط مسابقه یا race condition نخواهیم داشت.


- در فایل synch.c، در تابع `sema_init` ممکن است در تعیین `sema→value` شرایط مسابقه داشته‌ باشیم اما جلوگیری از این اتفاق بر عهده کاربر است.
- در توابعِِ
    ```
    void sema_down (struct semaphore *sema)
    bool sema_try_down (struct semaphore *sema)
    void sema_up (struct semaphore *sema)
    ```

تمام تغییرات روی متغیر‌ها بعد از غیرفعال کردن interruptها اتفاق می‌افتد پس در این حالت race condition نداریم.

- در تابع `lock_init` هم race condition نداریم چون تمام assignmentها به variableها مقدار ثابتی دارند.
- در تابع lock_acquire بعد از انجام تغییرات گفته شده در قسمت‌های قبل، باید هنگام تغییر priorityها interruptها را خاموش کنیم. همان‌طور که در داک گفته شده بود، حتی استفاده از lock هم نمی‌تواند تضمین کند که thread فعلی عوض نشود. به این ترتیب، به علت race conditionای که موقع بررسی و تغییر priorityها رخ می‌دهد، تصمیم گرفتیم برای این نواحی interruptها را خاموش کنیم. دلیل گفته شده البته، به هر ناحیه دیگری نیز تعمیم می‌یابد. یعنی هر جا که می‌خواهیم priority یک یا چند thread را بررسی کنیم یا تغییر دهیم، interruptها را خاموش می‌کنیم. در تابع `lock_acquire` باید به optimizationهای compiler هم توجه کنیم. تغییر ترتیب اجرای دستورات در برخی موارد ممکن است باعث به وجود آمدن deadlock شود. این موارد در حال حاضر بررسی شده‌اند.
- تابع `lock_try_acquire` شامل تغییرات در priority نمی‌شود. در پیاده‌سازی فعلی آن از `sema_try_down` استفاده شده که thread_safe است. در بقیه قسمت‌های این تابع هم تغییری روی متغیر global نداریم. بنابراین در این تابع race condition اتفاق نمی‌افتد.
- در تابع `lock_release` بعد از آزاد کردن lock باید تغییراتی در priority اعمال شود. بنابراین در این تابع interruptها را خاموش می‌کنیم.
- تابع `lock_held_by_current_thread` مشکلی از نظر race_condition ندارد.
- تابع `cond_init` هم مشکلی از نظر race condition ندارد.
- در تابع `cond_wait` هم از نظر race_condition مشکل داریم هم از نظر compiler optimization. در این تابع دو thread با دو lock متفاوت می‌توانند روی یک condition variable صبر یا wait کنند. از آن‌جا که در این شرایط push کردن روی لیست waiter از condition variable مشترک safe نیست، بهتر است در این تابع هم interruptها را غیرفعال کنیم.
    مشکل optimization هم زمانی پیش می‌آید که قبل از آزاد کردن lock روی semaphor تعریف شده sema_down کنیم. به این ترتیب lock هیچگاه آزاد نمی‌شود. بهتر است در این مواقع که ترتیب اجرا حائز اهمیت است از `()Barrier` استفاده کنیم.
- در دو تابع `cond_signal` و `cond_broadcast` تغییرات روی لیست داریم که مثل تابع `cond_wait` بهتر است در آن‌ها interruptها را خاموش کنیم.
## 4. منطق
- به طراحی‌های مختلفی برای زمان‌بند و به طور کلی تاثیر دادن priorityها فکر کردیم. اصلی‌ترین طراحی غیر از طراحی فعلی، شامل DFSزدن روی هر ترد برای محاسبه priority آن می‌شد. یعنی در scheduler و هر قسمت دیگر که می‌خواستیم thread با priority بالاتر را انتخاب کنیم، روی threadهای موجود DFS می‌زدیم تا effective priority محاسبه شود. DFS هم به این شکل بود که برای یک thread ابتدا روی لیست threadهایی که روی آن wait می‌کردند، iterate می‌کردیم و به صورت بازگشتی effective priority هر یک از آن‌ها را حساب می‌کردیم. بعد که effective priority تمام waiterها محاسبه شد، effective priority را برای thread اصلی محاسبه می‌کردیم. با وجود این‌که درستی این روش را قبول داشتیم، اجرای DFS برای تمام threadها در scheduler از $$O(n)$$ می‌شد که سربار زیادی داشت. ضمن این‌که تابع scheduler به دفعات صدا زده می‌شود و برای تغییر priorityها باید interruptها را خاموش می‌کردیم. به این ترتیب ممکن بود تعداد زیادی از timer tickها را از دست بدهیم.
- در ادامه متوجه شدیم زمانی که effective priority یک thread تغییر می‌کند، تنها لازم است effective priority مربوط threadای که روی آن wait می‌کرده را تغییر دهیم. اگر آن thread هم effective priorityاش تغییر کرد، تنها effective priority مربوط به threadای که روی آن wait می‌کرده را لازم است تغییر دهیم. اگر این کار را تا رسیدن به threadای که روی thread دیگری wait نمی‌کند انجام دهیم، می‌توانیم با صرف زمان کمتر effective priority تمام threadها را بروزرسانی کنیم. این روش علاوه بر ساده‌تر بودن، این امکان را به ما می‌دهد که تغییرات در priority را تنها زمانی اعمال کنیم که یک lock گرفته یا آزاد می‌شود. از آن جا که تعداد دفعات گرفته شدن و آزاد شدن lockها توسط threadهای مختلف بسیار کمتر از دفعاتی است که تابع scheduler صدا زده می‌شود، در مجموع عملکرد بهتری خواهیم داشت.
- در روش استفاده شده، تنها effective priority ترد‌هایی بررسی و تغییر داده می‌شود که ممکن است effective priority آن‌ها تغییر کند. بنابراین تعداد عملیات‌هایی که در این روش انجام می‌شود، می‌تواند بسیار کمتر از روش‌های دیگری باشد که تردها را بدون اطلاعات قبلی بررسی می‌کنند.
# سوالات افزون بر طراحی
1. در هنگام صدا شدن تابع `switch_threads`، ابتدا آدرس `struct thread` ریسه بعدی و سپس ریسه فعلی قرار می‌گیرد و در نهایت آدرس بازگشت (return address) می‌آید که همان مقدار ذخیره‌شده برای PC است. در تابع `switch_threads` ابتدا مقادیر registerها در پشته ریسه فعلی push می‌شوند و به کمک آدرس `struct thread` ریسه فعلی و `thread_stack_ofs`، مقدار stack pointer در بخش `stack` ذخیره می‌شود. بنابراین مقادیر PC و stack pointer و registers در پشته ریسه فعلی ذخیره شده‌است.
    
2. این صفحه در انتهای تابع `thread_schedule_tail` آزاد می‌شود که توسط تابع `schedule` صدا زده‌شده‌است. این آزاد‌سازی را نمی‌توان در تابع `thread_exit` انجام داد زیرا اولا این تابع در پشته ریسه فعلی در حال اجرا است و با آزادشدن پشته، آدرس بازگشت گم می‌شود. همچنین در تابع `switch_threads` به پشته ریسه فعلی احتیاج داریم. بنابراین این صفحه وقتی باید آزاد شود که ریسه بعدی در حال اجرا باشد.
    
3. تابع `thread_tick` در پشته ریسه فعلی اجرا می‌شود و در صورتی که زمان اجرای آن تمام شده‌باشد به `intr_handler` خبر می‌دهد که CPU را به ریسه‌ی دیگری بدهد.
    
4. برای این تست سناریوی زیر را می‌چینیم:
    - در ریسه اصلی تست که اولویت آن مقدار پیش‌فرض(31) است ابتدا یک semaphore و یک lock می‌سازیم. سپس ریسه `low` را با اولویت 32 و ریسه `med`‌ را با اولویت 33 می‌سازیم و `thread_yield` را صدا می‌زنیم.
        بعد از آن ریسه `high` با اولویت 34 را می‌سازیم و بار دیگر `thread_yield` را صدا می‌کنیم.
        حال `sema_up` کرده و باز هم `thread_yield` می‌کنیم.
        خط بالا را یک بار دیگر تکرار می‌کنیم تا مطمئن شویم هر دو ریسه `low` و `med` می‌توانند کار خود را ادامه دهند.
    - در ریسه `low` ابتدا `lock_acquire` و بعد `sema_down` را صدا می‌زنیم و در نهایت پیام `**thread low done**` را چاپ می‌کنیم.
    - در ریسه `med` تنها `sema_down` را صدا می‌زنیم و پیام `**thread med done**` را چاپ می‌کنیم.
    - در ریسه `high` ابتدا `lock_acquire` را صدا زده و بعد پیام `**thread high done**` را چاپ می‌کنیم.
    اگر تابع `sema_up` به درستی کار کند، ترتیب پیام‌ها مطابق زیر خواهد شد:
    thread low done
    thread high done
    thread med done
    اما اگر تابع موردنظر به درستی کار نکند، ریسه `med` زودتر از `low` از تابع `sema_down` خارج می‌شود و بنابراین خروجی مانند زیر می‌شود:
    thread med done
    thread low done
    thread high done

توجه شود که اولویت ریسه اصلی که در ارتباط با سایر ریسه‌ها است پایین‌تر از بقیه است تا تنها زمانی کد خود را ادامه دهد که دیگر ریسه‌ها **block** شده‌اند.

