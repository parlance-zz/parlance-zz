This project was a proof-of-concept for an effective key logger with remote data access that can run on modern Windows (7, 8, 10) in plain sight limited user permissions.
The keylogger itself is written in C++, there is a optional code injector program that can merge the keylogger payload into other executables, and that injector is written in C++ and x86 ASM.
The backend server is written in PHP and uses mySQL for storage.

I accept no liability for any use of the code. This code is only a proof-of-concept and is not intended to be used for any illegal activities.

As of Sept 27/2022 this code will actually still run on Windows 10 without triggering any security alerts. Only the most exotic engines on virus-total detect it as a potential positive, in spite of the fact that almost 0 effort is made to hide what it is doing. Be careful what you download!
