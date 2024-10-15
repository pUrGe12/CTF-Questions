# Finding hash type

use objdump on the file,

		objdump -d ./binary

you'll get a whole bunch of things, but we'll focus here,

		0000000000002829 <main>:

cause obviously, this is the main function.

I'll be using `binary ninja` simultaneously to draw conclusions on the assembly code. If you use ghidra or IDA then the output will be similar, it's just that I like binary ninja more.

## part 1 - Input size check

This is binary ninja's decompiled code,

		00002839      void* fsbase
		00002839      int64_t rax = *(fsbase + 0x28)
		0000285c      std::operator<<<std::char_traits<char> >(__out: &std::cout, __s: "Enter a string.\n")
		00002868      void __str
		00002868      std::string::string(this: &__str)
		0000287e      std::getline<char>(__is: &std::cin, &__str)
		00002893      std::string::size_type rax_1
		00002893      rax_1.b = std::string::length(this: &__str) u> 0x40
		00002898      int32_t result

The first two lines of code here are initialising the stack canary. (if you run `checksec` on this, then you'll see that canary is enabled). It is then calling `cout` on "Enter a string.", getting a line using `getline` and running a check on the size of the string, if its > 0x40 (that is 64). Its an unsigned comparision (so, I am guessing its comparing the absolute values, but I am not sure as well). It stores the string in the variable `rax_1.b` which is derived from the register `rax`. 

Before diving into assembly, you need to understand that decompilers use varied syntax. That is, the source and destination registers and values may be interchanged in some cases, but fret not. A quick google seach can help with this easily.

Let's compare this to the assembly code, through `objdump`. 

	    2829:	f3 0f 1e fa          	endbr64 
	    282d:	55                   	push   %rbp
	    282e:	48 89 e5             	mov    %rsp,%rbp
	    2831:	53                   	push   %rbx
	    2832:	48 81 ec f8 00 00 00 	sub    $0xf8,%rsp
	    2839:	64 48 8b 04 25 28 00 	mov    %fs:0x28,%rax
	    2840:	00 00 
	    2842:	48 89 45 e8          	mov    %rax,-0x18(%rbp)
	    2846:	31 c0                	xor    %eax,%eax
	    2848:	48 8d 05 d9 47 00 00 	lea    0x47d9(%rip),%rax        # 7028 <_ZL7show_Wt+0x1>
	    284f:	48 89 c6             	mov    %rax,%rsi
	    2852:	48 8d 05 27 88 00 00 	lea    0x8827(%rip),%rax        # b080 <_ZSt4cout@GLIBCXX_3.4>
	    2859:	48 89 c7             	mov    %rax,%rdi
	    285c:	e8 df fc ff ff       	call   2540 <_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc@plt>
	    2861:	48 8d 45 80          	lea    -0x80(%rbp),%rax
	    2865:	48 89 c7             	mov    %rax,%rdi
	    2868:	e8 c3 fd ff ff       	call   2630 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1Ev@plt>
	    286d:	48 8d 45 80          	lea    -0x80(%rbp),%rax
	    2871:	48 89 c6             	mov    %rax,%rsi
	    2874:	48 8d 05 25 89 00 00 	lea    0x8925(%rip),%rax        # b1a0 <_ZSt3cin@GLIBCXX_3.4>
	    287b:	48 89 c7             	mov    %rax,%rdi
	    287e:	e8 6d fb ff ff       	call   23f0 <_ZSt7getlineIcSt11char_traitsIcESaIcEERSt13basic_istreamIT_T0_ES7_RNSt7__cxx1112basic_stringIS4_S5_T1_EE@plt>
	    2883:	48 8d 45 80          	lea    -0x80(%rbp),%rax
	    2887:	48 89 c7             	mov    %rax,%rdi
	    288a:	e8 c1 fd ff ff       	call   2650 <_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6lengthEv@plt>
	    288f:	48 83 f8 40          	cmp    $0x40,%rax
	    2893:	0f 97 c0             	seta   %al
	    2896:	84 c0                	test   %al,%al
	    2898:	74 23                	je     28bd <main+0x94>

It may look like its much less readable, but we can actually infer much more from here. The addresses 2829 - 2839 describe the stack pointer initialising and holding the value of the canary. This canary is likely something `time` based and `RDTSC` based (you can look it up, it's a random value, but it probably is generated in this way).

We already know at address `285c` we have the `cout` function, and with the assembly we can see exactly how its implemented. We can infer that, the mangled string

		<_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc@plt>

is a `std::cout` in disguise (this will be helpful in the futuer if you can start making patterns!). To call `cout` you'll first have to load its effective address from Glibc++ (which basically stands for GNC Libc, since I am using Linux), this is the `lea` instruction that you see. So, we load its address, store it in the `rax` register, then move the stack pointer there and call it. 

The next thing we can notice is,

		<_ZSt7getlineIcSt11char_traitsIcESaIcEERSt13basic_istreamIT_T0_ES7_RNSt7__cxx1112basic_stringIS4_S5_T1_EE@plt>

is the mangled name for the `getline` command. Same thing here, we first load its address and then call it. You can see the instruction over at `288f` compares the length of this string to 0x40 (64). The way this comparision works out is actually pretty interesting!

	    288f:	48 83 f8 40          	cmp    $0x40,%rax
	    2893:	0f 97 c0             	seta   %al
	    2896:	84 c0                	test   %al,%al
	    2898:	74 23                	je     28bd <main+0x94>

`seta` stands for "set if above", that is, if the above compare instruction is true (which is checking for a greater than comprision cause of `seta`), then set the register 'al' as the first 8 bites of the general purpose register AX is the 16-bit intel processors. The important thing to note here, is the "jump if equal" condition, `je`. We are told that, if the compare instruction passes, then jump to this memory address, (that is at an offset of 0x94 from the address of the `main` function). If the check fails, then we are given these few lines of code,

		289a:	48 8d 05 9f 47 00 00 	lea    0x479f(%rip),%rax        # 7040 <_ZL7show_Wt+0x19>
	    28a1:	48 89 c6             	mov    %rax,%rsi
	    28a4:	48 8d 05 d5 87 00 00 	lea    0x87d5(%rip),%rax        # b080 <_ZSt4cout@GLIBCXX_3.4>
	    28ab:	48 89 c7             	mov    %rax,%rdi
	    28ae:	e8 8d fc ff ff       	call   2540 <_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc@plt>
	    28b3:	bb 01 00 00 00       	mov    $0x1,%ebx
	    28b8:	e9 88 02 00 00       	jmp    2b45 <main+0x31c>

This is what runs when the compare instruction fails. We can see that it's loading the address of `cout` again, so its going to print something, we then `mov` the value at `rdi` to `rax`, cause cout will be called on the value at `rax`. We've already loaded something in `rax` in instructions at 289a and 28a1. So, what happens after printing? We jump (`jmp`) to the address 2b45, and checking that address out we see this,

		2b45:	48 8d 45 80          	lea    -0x80(%rbp),%rax
		2b49:	48 89 c7             	mov    %rax,%rdi
		2b4c:	e8 3f f9 ff ff       	call   2490 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev@plt>
		2b51:	89 d8                	mov    %ebx,%eax
		2b53:	48 8b 55 e8          	mov    -0x18(%rbp),%rdx
		2b57:	64 48 2b 14 25 28 00 	sub    %fs:0x28,%rdx

This is basically just checking for stack overflows using the canary value. We can see the `sub` instruction over at the address where the canary is located. It's probably going to then check if this value is non-zero in which case there has been a binary overflow. This mangled string is representing the "std::string" initialiser,  `<_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev@plt>`. So, we can see that if we get the first check wrong, that is, if our input string is not less than or equal to 64 in length, then we will be pushed out of the program with a message. 

## part 2 - Figuring out the algorithm

Now let's go back to the main function again. This is the binary ninja's decompiled version

		00002898      if (rax_1.b == 0)
		000028c7          void var_108
		000028c7          std::vector<uint64_t>::vector(&var_108)
		000028da          void var_48
		000028da          std::string::string(this: &var_48)
		000028f0          void var_e8
		000028f0          convert_to_binary(&var_e8, &var_48)
		00002909          std::vector<uint64_t>::operator=(&var_108, &var_e8)
		00002918          std::vector<uint64_t>::~vector(&var_e8)
		00002924          std::string::~string(this: &var_48)
		0000293d          void var_a8
		0000293d          std::vector<uint64_t>::vector(&var_a8, &var_108)
		00002956          void var_c8
		00002956          pad_to_512bits(&var_c8, &var_a8)
		0000296f          std::vector<uint64_t>::operator=(&var_108, &var_c8)
		0000297e          std::vector<uint64_t>::~vector(&var_c8)
		0000298d          std::vector<uint64_t>::~vector(&var_a8)
		000029a6          std::vector<uint64_t>::vector(&var_c8, &var_108)
		000029bf          resize_block(&var_a8, &var_c8)
		000029d8          std::vector<uint64_t>::operator=(&var_108, &var_a8)
		000029e7          std::vector<uint64_t>::~vector(&var_a8)
		000029f6          std::vector<uint64_t>::~vector(&var_c8)
		00002a0f          std::vector<uint64_t>::vector(&var_a8, &var_108)
		00002a25          void __str_1
		00002a25          compute_hash(&__str_1, &var_a8)
		00002a34          std::vector<uint64_t>::~vector(&var_a8)
		00002a43          std::allocator<char>::allocator(this: &var_a8)
		00002a60          std::string::string<std::allocator<char> >(&var_48, "040d7f7bd376bbb5cb0d08663325a796…")
		00002a6f          std::allocator<char>::~allocator(this: &var_a8)
		00002a89          int32_t rax_2

		00002a89          rax_2.b = std::string::compare(this: &var_48, __str: &__str_1) == 0
		00002a8e          if (rax_2.b == 0)
		00002aec              std::operator<<<std::char_traits<char> >(__out: &std::cout, __s: "\tFAILED!\n")
		00002b14              std::ostream::operator<<(this: std::operator<<<char>(__os: &std::cout), __pf: std::endl<char>)
		00002b19              result = 1
		00002a8e          else
		00002aa4              std::operator<<<std::char_traits<char> >(__out: &std::cout, __s: "\tPASSED!\n")
		00002acc              std::ostream::operator<<(this: std::operator<<<char>(__os: &std::cout), __pf: std::endl<char>)
		00002ad1              result = 0

I hope this is as evident to you as it is to me. Whats happening here is, `rax_1.b` which holds the value of the previous comparision, is being tested against 0, and if it is, then we enter this loop. We already saw what happens when is it not equal to 0, (the `jmp 2b45` instruction). So, once inside (recalling that this happens when the length of the entered string is less than 64),

---

we're initialising some variables, making a vector, `coverting to binary`, then passing that to `pad_to_512bits`, passing that output to `resize_block` and then finally to the `compute_hash` function. Then it seems like we are loading a pre-defined hash and comparing our generated hash to that 

		rax_2.b = std::string::compare(this: &var_48, __str: &__str_1) == 0)

So, the program is taking some user input, hashing that using these functions and then comparing it to this value! Let's look at it from the assembly code,

	    28bd:	48 8d 85 00 ff ff ff 	lea    -0x100(%rbp),%rax
	    28c4:	48 89 c7             	mov    %rax,%rdi
	    28c7:	e8 20 19 00 00       	call   41ec <_ZNSt6vectorImSaImEEC1Ev>
	    28cc:	48 8d 55 80          	lea    -0x80(%rbp),%rdx
	    28d0:	48 8d 45 c0          	lea    -0x40(%rbp),%rax
	    28d4:	48 89 d6             	mov    %rdx,%rsi
	    28d7:	48 89 c7             	mov    %rax,%rdi
	    28da:	e8 61 fb ff ff       	call   2440 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1ERKS4_@plt>
	    28df:	48 8d 85 20 ff ff ff 	lea    -0xe0(%rbp),%rax
	    28e6:	48 8d 55 c0          	lea    -0x40(%rbp),%rdx
	    28ea:	48 89 d6             	mov    %rdx,%rsi
	    28ed:	48 89 c7             	mov    %rax,%rdi
	    28f0:	e8 95 08 00 00       	call   318a <_Z17convert_to_binaryNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE>
	    28f5:	48 8d 95 20 ff ff ff 	lea    -0xe0(%rbp),%rdx
	    28fc:	48 8d 85 00 ff ff ff 	lea    -0x100(%rbp),%rax
	    2903:	48 89 d6             	mov    %rdx,%rsi
	    2906:	48 89 c7             	mov    %rax,%rdi
	    2909:	e8 56 1c 00 00       	call   4564 <_ZNSt6vectorImSaImEEaSEOS1_>
	    290e:	48 8d 85 20 ff ff ff 	lea    -0xe0(%rbp),%rax
	    2915:	48 89 c7             	mov    %rax,%rdi
	    2918:	e8 ff 1b 00 00       	call   451c <_ZNSt6vectorImSaImEED1Ev>
	    291d:	48 8d 45 c0          	lea    -0x40(%rbp),%rax
	    2921:	48 89 c7             	mov    %rax,%rdi
	    2924:	e8 67 fb ff ff       	call   2490 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev@plt>
	    2929:	48 8d 95 00 ff ff ff 	lea    -0x100(%rbp),%rdx
	    2930:	48 8d 85 60 ff ff ff 	lea    -0xa0(%rbp),%rax
	    2937:	48 89 d6             	mov    %rdx,%rsi
	    293a:	48 89 c7             	mov    %rax,%rdi
	    293d:	e8 5e 1c 00 00       	call   45a0 <_ZNSt6vectorImSaImEEC1ERKS1_>
	    2942:	48 8d 85 40 ff ff ff 	lea    -0xc0(%rbp),%rax
	    2949:	48 8d 95 60 ff ff ff 	lea    -0xa0(%rbp),%rdx
	    2950:	48 89 d6             	mov    %rdx,%rsi
	    2953:	48 89 c7             	mov    %rax,%rdi
	    2956:	e8 16 09 00 00       	call   3271 <_Z14pad_to_512bitsSt6vectorImSaImEE>
	    295b:	48 8d 95 40 ff ff ff 	lea    -0xc0(%rbp),%rdx
	    2962:	48 8d 85 00 ff ff ff 	lea    -0x100(%rbp),%rax
	    2969:	48 89 d6             	mov    %rdx,%rsi
	    296c:	48 89 c7             	mov    %rax,%rdi
	    296f:	e8 f0 1b 00 00       	call   4564 <_ZNSt6vectorImSaImEEaSEOS1_>
	    2974:	48 8d 85 40 ff ff ff 	lea    -0xc0(%rbp),%rax
	    297b:	48 89 c7             	mov    %rax,%rdi
	    297e:	e8 99 1b 00 00       	call   451c <_ZNSt6vectorImSaImEED1Ev>
	    2983:	48 8d 85 60 ff ff ff 	lea    -0xa0(%rbp),%rax
	    298a:	48 89 c7             	mov    %rax,%rdi
	    298d:	e8 8a 1b 00 00       	call   451c <_ZNSt6vectorImSaImEED1Ev>
	    2992:	48 8d 95 00 ff ff ff 	lea    -0x100(%rbp),%rdx
	    2999:	48 8d 85 40 ff ff ff 	lea    -0xc0(%rbp),%rax
	    29a0:	48 89 d6             	mov    %rdx,%rsi
	    29a3:	48 89 c7             	mov    %rax,%rdi
	    29a6:	e8 f5 1b 00 00       	call   45a0 <_ZNSt6vectorImSaImEEC1ERKS1_>
	    29ab:	48 8d 85 60 ff ff ff 	lea    -0xa0(%rbp),%rax
	    29b2:	48 8d 95 40 ff ff ff 	lea    -0xc0(%rbp),%rdx
	    29b9:	48 89 d6             	mov    %rdx,%rsi
	    29bc:	48 89 c7             	mov    %rax,%rdi
	    29bf:	e8 81 02 00 00       	call   2c45 <_Z12resize_blockSt6vectorImSaImEE>
	    29c4:	48 8d 95 60 ff ff ff 	lea    -0xa0(%rbp),%rdx
	    29cb:	48 8d 85 00 ff ff ff 	lea    -0x100(%rbp),%rax
	    29d2:	48 89 d6             	mov    %rdx,%rsi
	    29d5:	48 89 c7             	mov    %rax,%rdi
	    29d8:	e8 87 1b 00 00       	call   4564 <_ZNSt6vectorImSaImEEaSEOS1_>
	    29dd:	48 8d 85 60 ff ff ff 	lea    -0xa0(%rbp),%rax
	    29e4:	48 89 c7             	mov    %rax,%rdi
	    29e7:	e8 30 1b 00 00       	call   451c <_ZNSt6vectorImSaImEED1Ev>
	    29ec:	48 8d 85 40 ff ff ff 	lea    -0xc0(%rbp),%rax
	    29f3:	48 89 c7             	mov    %rax,%rdi
	    29f6:	e8 21 1b 00 00       	call   451c <_ZNSt6vectorImSaImEED1Ev>
	    29fb:	48 8d 95 00 ff ff ff 	lea    -0x100(%rbp),%rdx
	    2a02:	48 8d 85 60 ff ff ff 	lea    -0xa0(%rbp),%rax
	    2a09:	48 89 d6             	mov    %rdx,%rsi
	    2a0c:	48 89 c7             	mov    %rax,%rdi
	    2a0f:	e8 8c 1b 00 00       	call   45a0 <_ZNSt6vectorImSaImEEC1ERKS1_>
	    2a14:	48 8d 45 a0          	lea    -0x60(%rbp),%rax
	    2a18:	48 8d 95 60 ff ff ff 	lea    -0xa0(%rbp),%rdx
	    2a1f:	48 89 d6             	mov    %rdx,%rsi
	    2a22:	48 89 c7             	mov    %rax,%rdi
	    2a25:	e8 c8 0a 00 00       	call   34f2 <_Z12compute_hashB5cxx11St6vectorImSaImEE>
	    2a2a:	48 8d 85 60 ff ff ff 	lea    -0xa0(%rbp),%rax
	    2a31:	48 89 c7             	mov    %rax,%rdi
	    2a34:	e8 e3 1a 00 00       	call   451c <_ZNSt6vectorImSaImEED1Ev>
	    2a39:	48 8d 85 60 ff ff ff 	lea    -0xa0(%rbp),%rax
	    2a40:	48 89 c7             	mov    %rax,%rdi
	    2a43:	e8 a8 fc ff ff       	call   26f0 <_ZNSaIcEC1Ev@plt>
	    2a48:	48 8d 95 60 ff ff ff 	lea    -0xa0(%rbp),%rdx
	    2a4f:	48 8d 45 c0          	lea    -0x40(%rbp),%rax
	    2a53:	48 8d 0d 16 46 00 00 	lea    0x4616(%rip),%rcx        # 7070 <_ZL7show_Wt+0x49>
	    2a5a:	48 89 ce             	mov    %rcx,%rsi
	    2a5d:	48 89 c7             	mov    %rax,%rdi
	    2a60:	e8 5b 1c 00 00       	call   46c0 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_>
	    2a65:	48 8d 85 60 ff ff ff 	lea    -0xa0(%rbp),%rax
	    2a6c:	48 89 c7             	mov    %rax,%rdi
	    2a6f:	e8 2c fb ff ff       	call   25a0 <_ZNSaIcED1Ev@plt>
	    2a74:	48 8d 55 a0          	lea    -0x60(%rbp),%rdx
	    2a78:	48 8d 45 c0          	lea    -0x40(%rbp),%rax
	    2a7c:	48 89 d6             	mov    %rdx,%rsi
	    2a7f:	48 89 c7             	mov    %rax,%rdi
	    2a82:	e8 49 f9 ff ff       	call   23d0 <_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7compareERKS4_@plt>
	    2a87:	85 c0                	test   %eax,%eax
	    2a89:	0f 94 c0             	sete   %al
	    2a8c:	84 c0                	test   %al,%al
	    2a8e:	74 48                	je     2ad8 <main+0x2af>
	    2a90:	48 8d 05 1a 46 00 00 	lea    0x461a(%rip),%rax        # 70b1 <_ZL7show_Wt+0x8a>
	    2a97:	48 89 c6             	mov    %rax,%rsi
	    2a9a:	48 8d 05 df 85 00 00 	lea    0x85df(%rip),%rax        # b080 <_ZSt4cout@GLIBCXX_3.4>
	    2aa1:	48 89 c7             	mov    %rax,%rdi
	    2aa4:	e8 97 fa ff ff       	call   2540 <_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc@plt>
	    2aa9:	48 8d 45 a0          	lea    -0x60(%rbp),%rax
	    2aad:	48 89 c6             	mov    %rax,%rsi
	    2ab0:	48 8d 05 c9 85 00 00 	lea    0x85c9(%rip),%rax        # b080 <_ZSt4cout@GLIBCXX_3.4>
	    2ab7:	48 89 c7             	mov    %rax,%rdi
	    2aba:	e8 71 fa ff ff       	call   2530 <_ZStlsIcSt11char_traitsIcESaIcEERSt13basic_ostreamIT_T0_ES7_RKNSt7__cxx1112basic_stringIS4_S5_T1_EE@plt>
	    2abf:	48 8b 15 0a 85 00 00 	mov    0x850a(%rip),%rdx        # afd0 <_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_@GLIBCXX_3.4>
	    2ac6:	48 89 d6             	mov    %rdx,%rsi
	    2ac9:	48 89 c7             	mov    %rax,%rdi
	    2acc:	e8 af fa ff ff       	call   2580 <_ZNSolsEPFRSoS_E@plt>
	    2ad1:	bb 00 00 00 00       	mov    $0x0,%ebx
	    2ad6:	eb 46                	jmp    2b1e <main+0x2f5>

Now thats a lot to digest, so let's look at it chunk wise,

		28bd:	48 8d 85 00 ff ff ff 	lea    -0x100(%rbp),%rax
		28c4:	48 89 c7             	mov    %rax,%rdi
		28c7:	e8 20 19 00 00       	call   41ec <_ZNSt6vectorImSaImEEC1Ev>
		28cc:	48 8d 55 80          	lea    -0x80(%rbp),%rdx
		28d0:	48 8d 45 c0          	lea    -0x40(%rbp),%rax
		28d4:	48 89 d6             	mov    %rdx,%rsi
		28d7:	48 89 c7             	mov    %rax,%rdi
		28da:	e8 61 fb ff ff       	call   2440 <_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1ERKS4_@plt>
		28df:	48 8d 85 20 ff ff ff 	lea    -0xe0(%rbp),%rax
		28e6:	48 8d 55 c0          	lea    -0x40(%rbp),%rdx
		28ea:	48 89 d6             	mov    %rdx,%rsi
		28ed:	48 89 c7             	mov    %rax,%rdi
		28f0:	e8 95 08 00 00       	call   318a <_Z17convert_to_binaryNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE>

This is the code before converting it to binary. Here is how you can understand this, firstly, we loads the address at the offset of 0x100 from the stack pointer into `rax`. This address is then loaded into `rdi` and the function at address `41ec` is called on this. We can see that, this is the vector initialisation (compare the decompiled code and this side by side). Moving on we can notice similar function calls, again and again, and finally at `28f0` when the function at `318a` is being called, which is named `convert_to_binary`. Let's actually take a look at this function,

		000000000000318a <_Z17convert_to_binaryNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE>:
				318a:	f3 0f 1e fa          	endbr64 
				318e:	55                   	push   %rbp
				318f:	48 89 e5             	mov    %rsp,%rbp
				3192:	53                   	push   %rbx
				3193:	48 83 ec 38          	sub    $0x38,%rsp
				3197:	48 89 7d c8          	mov    %rdi,-0x38(%rbp)
				319b:	48 89 75 c0          	mov    %rsi,-0x40(%rbp)
				319f:	64 48 8b 04 25 28 00 	mov    %fs:0x28,%rax
				31a6:	00 00 
				31a8:	48 89 45 e8          	mov    %rax,-0x18(%rbp)
				31ac:	31 c0                	xor    %eax,%eax
				31ae:	48 8b 45 c8          	mov    -0x38(%rbp),%rax
				31b2:	48 89 c7             	mov    %rax,%rdi
				31b5:	e8 32 10 00 00       	call   41ec <_ZNSt6vectorImSaImEEC1Ev>
				31ba:	c7 45 d4 00 00 00 00 	movl   $0x0,-0x2c(%rbp)
				31c1:	eb 54                	jmp    3217 <_Z17convert_to_binaryNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE+0x8d>
				31c3:	48 8b 45 c0          	mov    -0x40(%rbp),%rax
				31c7:	48 89 c7             	mov    %rax,%rdi
				31ca:	e8 61 f2 ff ff       	call   2430 <_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE5c_strEv@plt>
				31cf:	48 89 c2             	mov    %rax,%rdx
				31d2:	8b 45 d4             	mov    -0x2c(%rbp),%eax
				31d5:	48 98                	cltq   
				31d7:	48 01 d0             	add    %rdx,%rax
				31da:	0f b6 00             	movzbl (%rax),%eax
				31dd:	48 0f be d0          	movsbq %al,%rdx
				31e1:	48 8d 45 d8          	lea    -0x28(%rbp),%rax
				31e5:	48 89 d6             	mov    %rdx,%rsi
				31e8:	48 89 c7             	mov    %rax,%rdi
				31eb:	e8 bc 10 00 00       	call   42ac <_ZNSt6bitsetILm8EEC1Ey>
				31f0:	48 8d 45 d8          	lea    -0x28(%rbp),%rax
				31f4:	48 89 c7             	mov    %rax,%rdi
				31f7:	e8 ba 17 00 00       	call   49b6 <_ZNKSt6bitsetILm8EE8to_ulongEv>
				31fc:	48 89 45 e0          	mov    %rax,-0x20(%rbp)
				3200:	48 8d 55 e0          	lea    -0x20(%rbp),%rdx
				3204:	48 8b 45 c8          	mov    -0x38(%rbp),%rax
				3208:	48 89 d6             	mov    %rdx,%rsi
				320b:	48 89 c7             	mov    %rax,%rdi
				320e:	e8 c1 17 00 00       	call   49d4 <_ZNSt6vectorImSaImEE9push_backEOm>
				3213:	83 45 d4 01          	addl   $0x1,-0x2c(%rbp)
				3217:	8b 45 d4             	mov    -0x2c(%rbp),%eax
				321a:	48 63 d8             	movslq %eax,%rbx
				321d:	48 8b 45 c0          	mov    -0x40(%rbp),%rax
				3221:	48 89 c7             	mov    %rax,%rdi
				3224:	e8 87 f2 ff ff       	call   24b0 <_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE4sizeEv@plt>
				3229:	48 39 c3             	cmp    %rax,%rbx
				322c:	0f 92 c0             	setb   %al
				322f:	84 c0                	test   %al,%al
				3231:	75 90                	jne    31c3 <_Z17convert_to_binaryNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE+0x39>
				3233:	eb 1e                	jmp    3253 <_Z17convert_to_binaryNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE+0xc9>
				3235:	f3 0f 1e fa          	endbr64 
				3239:	48 89 c3             	mov    %rax,%rbx
				323c:	48 8b 45 c8          	mov    -0x38(%rbp),%rax
				3240:	48 89 c7             	mov    %rax,%rdi
				3243:	e8 d4 12 00 00       	call   451c <_ZNSt6vectorImSaImEED1Ev>
				3248:	48 89 d8             	mov    %rbx,%rax
				324b:	48 89 c7             	mov    %rax,%rdi
				324e:	e8 8d f4 ff ff       	call   26e0 <_Unwind_Resume@plt>
				3253:	48 8b 45 e8          	mov    -0x18(%rbp),%rax
				3257:	64 48 2b 04 25 28 00 	sub    %fs:0x28,%rax
				325e:	00 00 
				3260:	74 05                	je     3267 <_Z17convert_to_binaryNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE+0xdd>
				3262:	e8 69 f3 ff ff       	call   25d0 <__stack_chk_fail@plt>
				3267:	48 8b 45 c8          	mov    -0x38(%rbp),%rax
				326b:	48 8b 5d f8          	mov    -0x8(%rbp),%rbx
				326f:	c9                   	leave  
				3270:	c3                   	ret    

The first thing that stands out is the canary, (note: if you see something at an offset of 0x28 being loaded into `rax` its a good sign that there is a canary involved). Here is what I want you to notice

		31c3:	48 8b 45 c0          	mov    -0x40(%rbp),%rax
		31c7:	48 89 c7             	mov    %rax,%rdi
		31ca:	e8 61 f2 ff ff       	call   2430 <_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE5c_strEv@plt>
		31cf:	48 89 c2             	mov    %rax,%rdx
		31d2:	8b 45 d4             	mov    -0x2c(%rbp),%eax
		31d5:	48 98                	cltq   
		31d7:	48 01 d0             	add    %rdx,%rax
		31da:	0f b6 00             	movzbl (%rax),%eax
		31dd:	48 0f be d0          	movsbq %al,%rdx
		31e1:	48 8d 45 d8          	lea    -0x28(%rbp),%rax
		31e5:	48 89 d6             	mov    %rdx,%rsi
		31e8:	48 89 c7             	mov    %rax,%rdi
		31eb:	e8 bc 10 00 00       	call   42ac <_ZNSt6bitsetILm8EEC1Ey>
		31f0:	48 8d 45 d8          	lea    -0x28(%rbp),%rax
		31f4:	48 89 c7             	mov    %rax,%rdi
		31f7:	e8 ba 17 00 00       	call   49b6 <_ZNKSt6bitsetILm8EE8to_ulongEv>
		31fc:	48 89 45 e0          	mov    %rax,-0x20(%rbp)
		3200:	48 8d 55 e0          	lea    -0x20(%rbp),%rdx
		3204:	48 8b 45 c8          	mov    -0x38(%rbp),%rax
		3208:	48 89 d6             	mov    %rdx,%rsi
		320b:	48 89 c7             	mov    %rax,%rdi
		320e:	e8 c1 17 00 00       	call   49d4 <_ZNSt6vectorImSaImEE9push_backEOm>
		3213:	83 45 d4 01          	addl   $0x1,-0x2c(%rbp)
		3217:	8b 45 d4             	mov    -0x2c(%rbp),%eax
		321a:	48 63 d8             	movslq %eax,%rbx
		321d:	48 8b 45 c0          	mov    -0x40(%rbp),%rax
		3221:	48 89 c7             	mov    %rax,%rdi
		3224:	e8 87 f2 ff ff       	call   24b0 <_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE4sizeEv@plt>
		3229:	48 39 c3             	cmp    %rax,%rbx
		322c:	0f 92 c0             	setb   %al
		322f:	84 c0                	test   %al,%al
		3231:	75 90                	jne    31c3 <_Z17convert_to_binaryNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE+0x39>
		3233:	eb 1e                	jmp    3253 <_Z17convert_to_binaryNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE+0xc9>


At address `3231` you can see the jump if not equal instruction to memory `31c3`, notice that this is behind the address where this instruction is. This means a jump to the previous location, essentially, this is the structure for a loop!

The loop starts by loading the variable stored at `rbp-0x40` into memory (`rbp` is a frame pointer, so `rbp-0x40` means from the current address where the rbp points at, offset it by -0x40) into `rax`. This by itself won't give away that its a loop (cause its a pretty common practise).

What gives away that its a loop is 

a. The jump instruction to a previous location (that means everything is going to be executed again)
b. The increment by 1 (address `3213`)

The instruction `addl   $0x1,-0x2c(%rbp)` is adding one to the value stored at `rbp-0x2c`. Notice that we usually do this when we are inside a loop and we're incrementing by one each time to go to the next value (it can be a different value too)

So, usually when you see both of these together its a good indication that there is a loop involved in this function. Now, which loop is this exactly? This is made clear by this instruction, 

		jmp    3253

Why? Because if we check what is at `3253`,

		00003253  488b45e8           mov     rax, qword [rbp-0x18 {var_20}]
		00003257  64482b0425280000…  sub     rax, qword [fs:0x28]
		00003260  7405               je      0x3267
		00003262  e869f3ffff         call    __stack_chk_fail
		{ Does not return }
		00003267  488b45c8           mov     rax, qword [rbp-0x38 {var_40}]
		0000326b  488b5df8           mov     rbx, qword [rbp-0x8 {__saved_rbx}]
		0000326f  c9                 leave    {__saved_rbp}
		00003270  c3                 retn     {__return_addr}

It's either a stack failure or a return statement. This means, the `jump 	 3253` instruction is basically a break statement cause if it comes here then the function returns a value! This only happens in while loops (most of the times) and it might be a while true condition.

Additionally we can see,

		31f7:	e8 ba 17 00 00       	call   49b6 <_ZNKSt6bitsetILm8EE8to_ulongEv>

This is a bitset definition, of a 8 bit unsigned long. Now understanding the exact code from here is a little complex, so, let's look at the decompiled version.

		0000319f      void* fsbase
		0000319f      int64_t rax = *(fsbase + 0x28)
		000031b5      std::vector<uint64_t>::vector(arg1)
		000031ba      int32_t var_34 = 0
		000031ba      
		0000322c      while (true)
		0000322c          std::string::size_type rax_12
		0000322c          rax_12.b = sx.q(var_34) u< std::string::size(this: arg2)
		0000322c          
		00003231          if (rax_12.b == 0)
		00003231              break
		00003231          
		000031eb          void var_30
		000031eb          std::bitset<8ul>::bitset(&var_30, std::string::c_str(this: arg2)[sx.q(var_34)])
		000031fc          int64_t var_28 = std::bitset<8ul>::to_ulong(&var_30)
		0000320e          std::vector<uint64_t>::push_back(arg1, &var_28)
		00003213          var_34 += 1
		00003213      
		00003257      *(fsbase + 0x28)
		00003257      
		00003260      if (rax == *(fsbase + 0x28))
		00003270          return arg1
		00003270      
		00003262      __stack_chk_fail()
		00003262      noreturn

so, we're essentially taking 2 arguments, `arg1` and `arg2`, creating a vector from `arg1` (this was `31b5:	e8 32 10 00 00       	call   41ec <_ZNSt6vectorImSaImEEC1Ev>`). It then goes over every element in `arg2` and coverts that to an 8 bitset and pushes that to `arg1`. If there is no canary overflow it returns `arg1`. Bitset is basically converting characters to their ASCII equivalent binaries, so here essentially, we pass it a vector of charaters and it gives their ASCII values in binary.

We were pretty close with the assembly code as well to figuring this out! Actually, once you get to know bitset is being used and there is a loop with increments of 1, and a vector being used, then its a pretty guess to say that its a function that converts characters to their ASCII numbers in binary. 

The name probably gives it away as well ;).

---

# Hash cracking

once you've figured out the hashing algorithm, and you've obtained the hash for the password per se, you need to crack it using the pharases list given to you. 

If you want you can do this manually using (note the presence of `-n`)

		echo -n "<enter 1 phrase here>" | sha256sum

or you can write a bash script to read the file, hash each phrase using `sha256sum` and then compare it to the original one.

		#!/bin/bash
		target_hash="040d7f7bd376bbb5cb0d08663325a79609dbffabe575995bfb003e009d82cb1c"
		while IFS= read -r phrase
		do
		    phrase=$(echo -n "$phrase" | tr -d '\n')
		    hash=$(echo -n "$phrase" | sha256sum | awk '{ print $1 }')
		    if [ "$hash" == "$target_hash" ]; then
		        echo "$phrase"
		    fi
		done < phrases.txt

The best way would be to just use `hashcat`. Here, `hash.txt` contains the one single hash, the hash you obtained from the assembly.

		hashcat -m 1400 hash.txt phrases.txt

You will get the string: "one must imagine sisphus happy" (the typo is deliberate).-
