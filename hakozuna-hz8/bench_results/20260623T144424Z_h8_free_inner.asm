0000000000006110 <h8_free_inner>:
    6110:	f3 0f 1e fa          	endbr64 
    6114:	48 85 ff             	test   %rdi,%rdi
    6117:	0f 84 43 01 00 00    	je     6260 <h8_free_inner+0x150>
    611d:	48 8d 05 5c 5f 00 00 	lea    0x5f5c(%rip),%rax        # c080 <h8g>
    6124:	41 55                	push   %r13
    6126:	41 54                	push   %r12
    6128:	55                   	push   %rbp
    6129:	0f b6 50 08          	movzbl 0x8(%rax),%edx
    612d:	48 89 fd             	mov    %rdi,%rbp
    6130:	84 d2                	test   %dl,%dl
    6132:	0f 84 18 01 00 00    	je     6250 <h8_free_inner+0x140>
    6138:	48 8b 50 10          	mov    0x10(%rax),%rdx
    613c:	48 39 d7             	cmp    %rdx,%rdi
    613f:	0f 82 0b 01 00 00    	jb     6250 <h8_free_inner+0x140>
    6145:	48 8b 48 18          	mov    0x18(%rax),%rcx
    6149:	48 01 d1             	add    %rdx,%rcx
    614c:	48 39 cf             	cmp    %rcx,%rdi
    614f:	0f 83 fb 00 00 00    	jae    6250 <h8_free_inner+0x140>
    6155:	48 89 f9             	mov    %rdi,%rcx
    6158:	48 8b 40 38          	mov    0x38(%rax),%rax
    615c:	48 29 d1             	sub    %rdx,%rcx
    615f:	48 c1 e9 10          	shr    $0x10,%rcx
    6163:	4c 8b 24 c8          	mov    (%rax,%rcx,8),%r12
    6167:	4d 85 e4             	test   %r12,%r12
    616a:	0f 84 d4 00 00 00    	je     6244 <h8_free_inner+0x134>
    6170:	41 0f b6 44 24 4e    	movzbl 0x4e(%r12),%eax
    6176:	3c 05                	cmp    $0x5,%al
    6178:	0f 84 c6 00 00 00    	je     6244 <h8_free_inner+0x134>
    617e:	41 0f b7 4c 24 08    	movzwl 0x8(%r12),%ecx
    6184:	48 c7 c0 ff ff ff ff 	mov    $0xffffffffffffffff,%rax
    618b:	48 89 ea             	mov    %rbp,%rdx
    618e:	49 2b 14 24          	sub    (%r12),%rdx
    6192:	83 c1 04             	add    $0x4,%ecx
    6195:	48 d3 e0             	shl    %cl,%rax
    6198:	48 f7 d0             	not    %rax
    619b:	48 85 d0             	test   %rdx,%rax
    619e:	0f 85 a0 00 00 00    	jne    6244 <h8_free_inner+0x134>
    61a4:	41 0f b7 44 24 0a    	movzwl 0xa(%r12),%eax
    61aa:	48 d3 ea             	shr    %cl,%rdx
    61ad:	49 89 d5             	mov    %rdx,%r13
    61b0:	48 39 c2             	cmp    %rax,%rdx
    61b3:	0f 83 8b 00 00 00    	jae    6244 <h8_free_inner+0x134>
    61b9:	48 c7 c0 f8 ff ff ff 	mov    $0xfffffffffffffff8,%rax
    61c0:	64 48 8b 00          	mov    %fs:(%rax),%rax
    61c4:	48 85 c0             	test   %rax,%rax
    61c7:	0f 84 c7 00 00 00    	je     6294 <h8_free_inner+0x184>
    61cd:	48 8b 10             	mov    (%rax),%rdx
    61d0:	8b 72 08             	mov    0x8(%rdx),%esi
    61d3:	41 39 74 24 48       	cmp    %esi,0x48(%r12)
    61d8:	0f 85 8a 00 00 00    	jne    6268 <h8_free_inner+0x158>
    61de:	41 0f b7 4c 24 4c    	movzwl 0x4c(%r12),%ecx
    61e4:	3b 4a 0c             	cmp    0xc(%rdx),%ecx
    61e7:	75 7f                	jne    6268 <h8_free_inner+0x158>
    61e9:	41 0f b6 54 24 4e    	movzbl 0x4e(%r12),%edx
    61ef:	84 d2                	test   %dl,%dl
    61f1:	75 75                	jne    6268 <h8_free_inner+0x158>
    61f3:	49 8b 54 24 20       	mov    0x20(%r12),%rdx
    61f8:	42 8b 14 aa          	mov    (%rdx,%r13,4),%edx
    61fc:	c1 ea 1e             	shr    $0x1e,%edx
    61ff:	83 fa 01             	cmp    $0x1,%edx
    6202:	75 64                	jne    6268 <h8_free_inner+0x158>
    6204:	49 8b 54 24 18       	mov    0x18(%r12),%rdx
    6209:	4c 89 e9             	mov    %r13,%rcx
    620c:	48 c1 e9 06          	shr    $0x6,%rcx
    6210:	48 8b 14 ca          	mov    (%rdx,%rcx,8),%rdx
    6214:	4c 0f a3 ea          	bt     %r13,%rdx
    6218:	72 4e                	jb     6268 <h8_free_inner+0x158>
    621a:	41 8b 94 24 84 00 00 00 	mov    0x84(%r12),%edx
    6222:	49 8b 4c 24 20       	mov    0x20(%r12),%rcx
    6227:	81 ca 00 00 00 80    	or     $0x80000000,%edx
    622d:	42 89 14 a9          	mov    %edx,(%rcx,%r13,4)
    6231:	45 89 ac 24 84 00 00 00 	mov    %r13d,0x84(%r12)
    6239:	41 0f b7 54 24 08    	movzwl 0x8(%r12),%edx
    623f:	4c 89 64 d0 08       	mov    %r12,0x8(%rax,%rdx,8)
    6244:	5d                   	pop    %rbp
    6245:	41 5c                	pop    %r12
    6247:	41 5d                	pop    %r13
    6249:	c3                   	ret    
    624a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
    6250:	48 89 ef             	mov    %rbp,%rdi
    6253:	5d                   	pop    %rbp
    6254:	41 5c                	pop    %r12
    6256:	41 5d                	pop    %r13
    6258:	e9 83 00 00 00       	jmp    62e0 <h8_sys_free>
    625d:	0f 1f 00             	nopl   (%rax)
    6260:	c3                   	ret    
    6261:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
    6268:	4c 89 ee             	mov    %r13,%rsi
    626b:	4c 89 e7             	mov    %r12,%rdi
    626e:	e8 7d f1 ff ff       	call   53f0 <h8_remote_free_publish_known>
    6273:	83 f8 04             	cmp    $0x4,%eax
    6276:	75 cc                	jne    6244 <h8_free_inner+0x134>
    6278:	eb 0b                	jmp    6285 <h8_free_inner+0x175>
    627a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
    6280:	e8 bb b0 ff ff       	call   1340 <sched_yield@plt>
    6285:	48 89 ef             	mov    %rbp,%rdi
    6288:	e8 a3 f4 ff ff       	call   5730 <h8_remote_free_publish>
    628d:	83 f8 04             	cmp    $0x4,%eax
    6290:	74 ee                	je     6280 <h8_free_inner+0x170>
    6292:	eb b0                	jmp    6244 <h8_free_inner+0x134>
    6294:	e8 77 d9 ff ff       	call   3c10 <h8_thread_ctx_get_slow>
    6299:	48 85 c0             	test   %rax,%rax
    629c:	74 a6                	je     6244 <h8_free_inner+0x134>
    629e:	e9 2a ff ff ff       	jmp    61cd <h8_free_inner+0xbd>
    62a3:	66 2e 0f 1f 84 00 00 00 00 00 	cs nopw 0x0(%rax,%rax,1)
    62ad:	0f 1f 00             	nopl   (%rax)

