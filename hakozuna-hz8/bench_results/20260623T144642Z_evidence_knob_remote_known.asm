0000000000005380 <h8_remote_free_publish_known>:
    5380:	f3 0f 1e fa          	endbr64 
    5384:	41 55                	push   %r13
    5386:	41 54                	push   %r12
    5388:	55                   	push   %rbp
    5389:	48 89 fd             	mov    %rdi,%rbp
    538c:	53                   	push   %rbx
    538d:	48 89 f3             	mov    %rsi,%rbx
    5390:	48 83 ec 08          	sub    $0x8,%rsp
    5394:	48 8b 77 40          	mov    0x40(%rdi),%rsi
    5398:	48 89 f0             	mov    %rsi,%rax
    539b:	83 e0 3f             	and    $0x3f,%eax
    539e:	48 8d 15 db 6c 00 00 	lea    0x6cdb(%rip),%rdx        # c080 <h8g>
    53a5:	48 69 c0 38 01 00 00 	imul   $0x138,%rax,%rax
    53ac:	4c 8d 6c 10 40       	lea    0x40(%rax,%rdx,1),%r13
    53b1:	48 89 f0             	mov    %rsi,%rax
    53b4:	83 e0 3f             	and    $0x3f,%eax
    53b7:	48 69 c0 38 01 00 00 	imul   $0x138,%rax,%rax
    53be:	80 7c 02 50 00       	cmpb   $0x0,0x50(%rdx,%rax,1)
    53c3:	74 2b                	je     53f0 <h8_remote_free_publish_known+0x70>
    53c5:	e8 46 ec ff ff       	call   4010 <h8_span_publish_enter>
    53ca:	84 c0                	test   %al,%al
    53cc:	0f 85 c6 00 00 00    	jne    5498 <h8_remote_free_publish_known+0x118>
    53d2:	48 83 c4 08          	add    $0x8,%rsp
    53d6:	41 bc 04 00 00 00    	mov    $0x4,%r12d
    53dc:	5b                   	pop    %rbx
    53dd:	44 89 e0             	mov    %r12d,%eax
    53e0:	5d                   	pop    %rbp
    53e1:	41 5c                	pop    %r12
    53e3:	41 5d                	pop    %r13
    53e5:	c3                   	ret    
    53e6:	66 2e 0f 1f 84 00 00 00 00 00 	cs nopw 0x0(%rax,%rax,1)
    53f0:	48 c1 ee 06          	shr    $0x6,%rsi
    53f4:	4c 89 ef             	mov    %r13,%rdi
    53f7:	0f b7 f6             	movzwl %si,%esi
    53fa:	e8 71 f7 ff ff       	call   4b70 <h8_owner_publish_enter>
    53ff:	84 c0                	test   %al,%al
    5401:	74 cf                	je     53d2 <h8_remote_free_publish_known+0x52>
    5403:	48 8b 45 20          	mov    0x20(%rbp),%rax
    5407:	48 8b 75 18          	mov    0x18(%rbp),%rsi
    540b:	41 bc 02 00 00 00    	mov    $0x2,%r12d
    5411:	8b 04 98             	mov    (%rax,%rbx,4),%eax
    5414:	c1 e8 1e             	shr    $0x1e,%eax
    5417:	83 f8 01             	cmp    $0x1,%eax
    541a:	75 5f                	jne    547b <h8_remote_free_publish_known+0xfb>
    541c:	49 89 d8             	mov    %rbx,%r8
    541f:	89 d9                	mov    %ebx,%ecx
    5421:	ba 01 00 00 00       	mov    $0x1,%edx
    5426:	41 89 d9             	mov    %ebx,%r9d
    5429:	49 c1 e8 06          	shr    $0x6,%r8
    542d:	48 d3 e2             	shl    %cl,%rdx
    5430:	41 83 e1 3f          	and    $0x3f,%r9d
    5434:	4a 8d 0c c6          	lea    (%rsi,%r8,8),%rcx
    5438:	48 8b 01             	mov    (%rcx),%rax
    543b:	48 89 c7             	mov    %rax,%rdi
    543e:	48 89 c6             	mov    %rax,%rsi
    5441:	48 09 d7             	or     %rdx,%rdi
    5444:	f0 48 0f b1 39       	lock cmpxchg %rdi,(%rcx)
    5449:	75 f0                	jne    543b <h8_remote_free_publish_known+0xbb>
    544b:	41 bc 03 00 00 00    	mov    $0x3,%r12d
    5451:	48 85 f2             	test   %rsi,%rdx
    5454:	75 25                	jne    547b <h8_remote_free_publish_known+0xfb>
    5456:	48 8b 45 20          	mov    0x20(%rbp),%rax
    545a:	8b 04 98             	mov    (%rax,%rbx,4),%eax
    545d:	c1 e8 1e             	shr    $0x1e,%eax
    5460:	83 f8 01             	cmp    $0x1,%eax
    5463:	0f 84 b7 00 00 00    	je     5520 <h8_remote_free_publish_known+0x1a0>
    5469:	0f b6 45 52          	movzbl 0x52(%rbp),%eax
    546d:	83 e8 02             	sub    $0x2,%eax
    5470:	3c 01                	cmp    $0x1,%al
    5472:	0f 87 f8 00 00 00    	ja     5570 <h8_remote_free_publish_known+0x1f0>
    5478:	45 31 e4             	xor    %r12d,%r12d
    547b:	4c 89 ef             	mov    %r13,%rdi
    547e:	e8 fd f6 ff ff       	call   4b80 <h8_owner_publish_exit>
    5483:	48 83 c4 08          	add    $0x8,%rsp
    5487:	44 89 e0             	mov    %r12d,%eax
    548a:	5b                   	pop    %rbx
    548b:	5d                   	pop    %rbp
    548c:	41 5c                	pop    %r12
    548e:	41 5d                	pop    %r13
    5490:	c3                   	ret    
    5491:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
    5498:	48 8b 45 20          	mov    0x20(%rbp),%rax
    549c:	48 8b 75 18          	mov    0x18(%rbp),%rsi
    54a0:	41 bc 02 00 00 00    	mov    $0x2,%r12d
    54a6:	8b 04 98             	mov    (%rax,%rbx,4),%eax
    54a9:	c1 e8 1e             	shr    $0x1e,%eax
    54ac:	83 f8 01             	cmp    $0x1,%eax
    54af:	75 55                	jne    5506 <h8_remote_free_publish_known+0x186>
    54b1:	49 89 d8             	mov    %rbx,%r8
    54b4:	89 d9                	mov    %ebx,%ecx
    54b6:	ba 01 00 00 00       	mov    $0x1,%edx
    54bb:	41 89 d9             	mov    %ebx,%r9d
    54be:	49 c1 e8 06          	shr    $0x6,%r8
    54c2:	48 d3 e2             	shl    %cl,%rdx
    54c5:	41 83 e1 3f          	and    $0x3f,%r9d
    54c9:	4a 8d 0c c6          	lea    (%rsi,%r8,8),%rcx
    54cd:	48 8b 01             	mov    (%rcx),%rax
    54d0:	48 89 c7             	mov    %rax,%rdi
    54d3:	48 89 c6             	mov    %rax,%rsi
    54d6:	48 09 d7             	or     %rdx,%rdi
    54d9:	f0 48 0f b1 39       	lock cmpxchg %rdi,(%rcx)
    54de:	75 f0                	jne    54d0 <h8_remote_free_publish_known+0x150>
    54e0:	41 bc 03 00 00 00    	mov    $0x3,%r12d
    54e6:	48 85 f2             	test   %rsi,%rdx
    54e9:	75 1b                	jne    5506 <h8_remote_free_publish_known+0x186>
    54eb:	48 8b 45 20          	mov    0x20(%rbp),%rax
    54ef:	8b 04 98             	mov    (%rax,%rbx,4),%eax
    54f2:	c1 e8 1e             	shr    $0x1e,%eax
    54f5:	83 f8 01             	cmp    $0x1,%eax
    54f8:	75 56                	jne    5550 <h8_remote_free_publish_known+0x1d0>
    54fa:	48 85 f6             	test   %rsi,%rsi
    54fd:	0f 84 8d 00 00 00    	je     5590 <h8_remote_free_publish_known+0x210>
    5503:	45 31 e4             	xor    %r12d,%r12d
    5506:	48 89 ef             	mov    %rbp,%rdi
    5509:	e8 f2 ea ff ff       	call   4000 <h8_span_publish_exit>
    550e:	48 83 c4 08          	add    $0x8,%rsp
    5512:	44 89 e0             	mov    %r12d,%eax
    5515:	5b                   	pop    %rbx
    5516:	5d                   	pop    %rbp
    5517:	41 5c                	pop    %r12
    5519:	41 5d                	pop    %r13
    551b:	c3                   	ret    
    551c:	0f 1f 40 00          	nopl   0x0(%rax)
    5520:	48 85 f6             	test   %rsi,%rsi
    5523:	0f 85 4f ff ff ff    	jne    5478 <h8_remote_free_publish_known+0xf8>
    5529:	44 89 c1             	mov    %r8d,%ecx
    552c:	b8 01 00 00 00       	mov    $0x1,%eax
    5531:	48 d3 e0             	shl    %cl,%rax
    5534:	f0 48 09 45 60       	lock or %rax,0x60(%rbp)
    5539:	48 89 ee             	mov    %rbp,%rsi
    553c:	4c 89 ef             	mov    %r13,%rdi
    553f:	e8 2c fd ff ff       	call   5270 <h8_span_notify>
    5544:	e9 2f ff ff ff       	jmp    5478 <h8_remote_free_publish_known+0xf8>
    5549:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
    5550:	0f b6 45 52          	movzbl 0x52(%rbp),%eax
    5554:	83 e8 02             	sub    $0x2,%eax
    5557:	3c 01                	cmp    $0x1,%al
    5559:	76 a8                	jbe    5503 <h8_remote_free_publish_known+0x183>
    555b:	45 89 c9             	mov    %r9d,%r9d
    555e:	45 31 e4             	xor    %r12d,%r12d
    5561:	f0 4c 0f b3 09       	lock btr %r9,(%rcx)
    5566:	41 0f 92 c4          	setb   %r12b
    556a:	45 01 e4             	add    %r12d,%r12d
    556d:	eb 97                	jmp    5506 <h8_remote_free_publish_known+0x186>
    556f:	90                   	nop
    5570:	45 89 c9             	mov    %r9d,%r9d
    5573:	45 31 e4             	xor    %r12d,%r12d
    5576:	f0 4c 0f b3 09       	lock btr %r9,(%rcx)
    557b:	41 0f 92 c4          	setb   %r12b
    557f:	45 01 e4             	add    %r12d,%r12d
    5582:	e9 f4 fe ff ff       	jmp    547b <h8_remote_free_publish_known+0xfb>
    5587:	66 0f 1f 84 00 00 00 00 00 	nopw   0x0(%rax,%rax,1)
    5590:	44 89 c1             	mov    %r8d,%ecx
    5593:	b8 01 00 00 00       	mov    $0x1,%eax
    5598:	48 d3 e0             	shl    %cl,%rax
    559b:	f0 48 09 45 60       	lock or %rax,0x60(%rbp)
    55a0:	48 89 ee             	mov    %rbp,%rsi
    55a3:	4c 89 ef             	mov    %r13,%rdi
    55a6:	e8 c5 fc ff ff       	call   5270 <h8_span_notify>
    55ab:	e9 53 ff ff ff       	jmp    5503 <h8_remote_free_publish_known+0x183>

