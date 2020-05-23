تمرین گروهی ۳ - مستند طراحی
======================

گروه
-----
>>‫رضا یاربخش ryarbakhsh@gmail.com


مقدمات
----------
>>‫ ‫‫اگر نکات اضافه‌ای در مورد تمرین یا برای دستیاران آموزشی دارید در این قسمت
>> ‫بنویسید.

>>‫ لطفا در این قسمت تمامی منابعی (غیر از مستندات Pintos، اسلاید‌ها و دیگر منابع
>>‫ ‫درس) را که برای تمرین از آن‌ها استفاده کرده‌اید در این قسمت بنویسید.

بافر کش
============

داده‌ساختار‌ها و توابع
---------------------
>>‫ در این قسمت تعریف هر یک از `struct` ها، اعضای `struct` ها، متغیرهای سراسری
>>‫ یا ایستا، `typedef` ها یا `enum` هایی که ایجاد کرده‌اید یا تغییر داده‌اید را
>>‫ بنویسید و دلیل هر کدام را در حداکثر ۲۵ کلمه توضیح دهید.

الگوریتم‌ها
------------
>>‫ توضیح دهید که الگوریتم مورد استفاده‌ی شما به چه صورت یک بلاک را برای جایگزین 
>>‫ شدن انتخاب می‌کند؟

>>‫ روش پیاده‌سازی `read-ahead` را توضیح دهید.

همگام سازی
-------------
>>‫ هنگامی که یک پردازه به طور مستمر در حال خواندن یا نوشتن داده در یک بلاک بافرکش
>>‫ می‌باشد به چه صورت از دخالت سایر پردازه‌ها جلوگیری میشود؟

>>‫ در حین خارج شدن یک بلوک از حافظه‌ی نهان، چگونه از پروسه‌های دیگر جلوگیری می‌شود تا
>>‫ به این بلاک دسترسی پیدا نکنند؟

منطق طراحی
-----------------
>>‫ یک سناریو را توضیح دهید که از بافر کش، `read-ahead` و یا از `write-behind` استفاده کند.


فایل‌های قابل گسترش
=====================

داده‌ساختار‌ها و توابع
---------------------
>>‫ در این قسمت تعریف هر یک از `struct` ها، اعضای `struct` ها، متغیرهای سراسری
>>‫ یا ایستا، `typedef` ها یا `enum` هایی که ایجاد کرده‌اید یا تغییر داده‌اید را بنویسید و
>>‫ دلیل هر کدام را در حداکثر ۲۵ کلمه توضیح دهید.

>>‫ بیشترین سایز فایل پشتیبانی شده توسط ساختار inode شما چقدر است؟

همگام سازی
----------
>>‫ توضیح دهید که اگر دو پردازه بخواهند یک فایل را به طور همزمان گسترش دهند، کد شما چگونه از
>>‫ حالت مسابقه جلوگیری می‌کند.

>>‫ فرض کنید دو پردازه‌ی A و B فایل F را باز کرده‌اند و هر دو به end-of-file اشاره کرده‌اند.
>>‫ اگر  همزمان A از F بخواند و B روی آن بنویسد، ممکن است که A تمام، بخشی یا هیچ چیز از
>>‫ اطلاعات نوشته شده توسط B را بخواند. همچنین A نمی‌تواند چیزی جز اطلاعات نوشته شده توسط B را
>>‫ بخواند. مثلا اگر B تماما ۱ بنویسد، A نیز باید تماما ۱ بخواند. توضیح دهید کد شما چگونه از
>>‫ این حالت مسابقه جلوگیری می‌کند.

>>‫ توضیح دهید همگام سازی شما چگونه "عدالت" را برقرار می‌کند. فایل سیستمی "عادل" است که
>>‫ خواننده‌های اطلاعات به صورت ناسازگار نویسنده‌های اطلاعات را مسدود نکنند و برعکس. بدین ترتیب
>>‫ اگر تعدادی بسیار زیاد پردازه‌هایی که از یک فایل می‌خوانند نمی‌توانند تا ابد مانع نوشده شدن
>>‫ اطلاعات توسط یک پردازه‌ی دیگر شوند و برعکس.

منطق طراحی
----------
>>‫ آیا ساختار `inode` شما از طبقه‌بندی چند سطحه پشتیبانی می‌کند؟ اگر بله، دلیل خود را برای
>>‫ انتخاب این ترکیب خاص از بلوک‌های مستقیم، غیر مستقیم و غیر مستقیم دوطرفه توضیح دهید.
>>‌‫ اگر خیر، دلیل خود برای انتخاب ساختاری غیر از طبقه‌بندی چند سطحه و مزایا و معایب ساختار
>>‫ مورد استفاده خود نسبت به طبقه‌بندی چند سطحه را توضیح دهید.

زیرمسیرها
============

داده‌ساختار‌ها و توابع
---------------------
>>‫ در این قسمت تعریف هر یک از `struct` ها، اعضای `struct` ها، متغیرهای سراسری
>>‫ یا ایستا، `typedef` ها یا `enum` هایی که ایجاد کرده‌اید یا تغییر داده‌اید را بنویسید و
>>‫ دلیل هر کدام را در حداکثر ۲۵ کلمه توضیح دهید.

در فایل `node.c` به `struct inode_disk` موارد زیر را اضافه میکنیم:

`bool is_dir`: جهت مشخص کردن پوشه بودن یک inode

`block_sector_t parent`: جهت نگهداری پدر یک inode

به `struct thread` در `thread.c`  جهت نگهداری پوشه کار فعلی `struct dir *cwd` را  اضافه میکنیم.

به `struct dir` در `directory.c` جهت جلوگیری از مسابقه `struct lock dir_lock` را اضافه میکنیم.

به دو تابع `inode create` و `filesys_create` جهت تشخیص پوشه بودن `bool is_dir` را اضافه میکنیم.


الگوریتم‌ها
------------
>>‫ کد خود را برای طی کردن یک مسیر گرفته‌شده از کاربر را توضیح دهید.
>>‫ آیا عبور از مسیرهای absolute و relative تفاوتی دارد؟

ابتدا حرف اول  را بررسی میکنیم در صورتی که / بود از root شروع میکنیم و در غیر اینصورت از cwd شروع میکنیم. سپس حروف را تا زمانی که به / برسیم خوانده و ادامه میدهیم. در صورتی که پوشه ای با این نام وجود داشت آن را پیمایش کرده , در صورتی که .. بود به سمت پوشه پدر رفته و در صورتی که نام فایل بود بایستی آخرین قسمت آدرس باشد.

همگام سازی
-------------
>>‫ چگونه از رخ دادن race-condition در مورد دایرکتوری ها پیشگیری می‌کنید؟
>>‫ برای مثال اگر دو درخواست موازی برای حذف یک فایل وجود داشته باشد و 
>>‫ تنها یکی از آنها باید موفق شود یا مثلاً دو ریسه موازی بخواهند فایلی
>>‫ یک اسم در یک مسیر ایجاد کنند و مانند آن.

با اضافه شدن قفل به `struct dir` کارهایی مثل حذف فایل و یا اضافه کردن فایل به هر پوشه را امن ریسه میکنیم.
>>‫ آیا پیاده سازی شما اجازه می‌دهد مسیری که CWD یک ریسه شده یا پردازه‌ای
>>‫ از آن استفاده می‌کند حذف شود؟ اگر بله، عملیات فایل سیستم بعدی روی آن
>>‫ دایرکتوری چه نتیجه‌ای می‌دهند؟ اگر نه، چطور جلوی آن را می‌گیرید؟

هنگامی که `cwd` یک ریسه را تعیین میکنیم, پوشه را باز میکنیم و `open_cnt` آن یکی زیاد میشود. در صورتی که پوشه‌ای `open_cnt` بیشتر از 0 داشته باشد اجازه حذف آن را نمیدهیم.

منطق طراحی
-----------------
>>‫ توضیح دهید چرا تصمیم گرفتید CWD یک پردازه را به شکلی که طراحی کرده‌اید
>>‫ پیاده‌سازی کنید؟

به جای استفاده از `inode` برای `cwd` از `dir` استفاده کردیم. چرا که استفاده از api برای `dir` ساده تر است و باز و بسته کردن آن ساده تر از `inode` میباشد.

### سوالات نظرسنجی

پاسخ به این سوالات دلخواه است، اما به ما برای بهبود این درس در ادامه کمک خواهد کرد.
نظرات خود را آزادانه به ما بگوئید—این سوالات فقط برای سنجش افکار شماست.
ممکن است شما بخواهید ارزیابی خود از درس را به صورت ناشناس و در انتهای ترم بیان کنید.

>>‫ به نظر شما، این تمرین گروهی، یا هر کدام از سه وظیفه آن، از نظر دشواری در چه سطحی بود؟ خیلی سخت یا خیلی آسان؟
چه مدت زمانی را صرف انجام این تمرین کردید؟ نسبتا زیاد یا خیلی کم؟

>>‫ آیا بعد از کار بر روی یک بخش خاص از این تمرین (هر بخشی)، این احساس در شما به وجود آمد که اکنون یک دید بهتر نسبت به برخی جنبه‌های سیستم عامل دارید؟

>>‫ آیا نکته یا راهنمایی خاصی وجود دارد که بهتر است ما آنها را به توضیحات این تمرین اضافه کنیم تا به دانشجویان ترم های آتی در حل مسائل کمک کند؟
متقابلا، آیا راهنمایی نادرستی که منجر به گمراهی شما شود وجود داشته است؟

>>‫ آیا پیشنهادی در مورد دستیاران آموزشی درس، برای همکاری موثرتر با دانشجویان دارید؟
این پیشنهادات میتوانند هم برای تمرین‌های گروهی بعدی همین ترم و هم برای ترم‌های آینده باشد.

>>‫ آیا حرف دیگری دارید؟