00000000000053f0 <h8_remote_free_publish_known>:
    53f0:	f3 0f 1e fa          	endbr64 
    53f4:	41 55                	push   %r13
    53f6:	41 54                	push   %r12
    53f8:	55                   	push   %rbp
    53f9:	48 89 fd             	mov    %rdi,%rbp
    53fc:	53                   	push   %rbx
    53fd:	48 89 f3             	mov    %rsi,%rbx
    5400:	48 83 ec 08          	sub    $0x8,%rsp
    5404:	48 8b 77 40          	mov    0x40(%rdi),%rsi
    5408:	48 89 f0             	mov    %rsi,%rax
    540b:	83 e0 3f             	and    $0x3f,%eax
    540e:	4c 8d 25 6b 6c 00 00 	lea    0x6c6b(%rip),%r12        # c080 <h8g>
    5415:	48 69 c0 38 01 00 00 	imul   $0x138,%rax,%rax
    541c:	4e 8d 6c 20 40       	lea    0x40(%rax,%r12,1),%r13
    5421:	48 89 f0             	mov    %rsi,%rax
    5424:	83 e0 3f             	and    $0x3f,%eax
    5427:	48 69 c0 38 01 00 00 	imul   $0x138,%rax,%rax
    542e:	41 80 7c 04 50 00    	cmpb   $0x0,0x50(%r12,%rax,1)
    5434:	74 2a                	je     5460 <h8_remote_free_publish_known+0x70>
    5436:	e8 45 ec ff ff       	call   4080 <h8_span_publish_enter>
    543b:	84 c0                	test   %al,%al
    543d:	0f 85 fd 00 00 00    	jne    5540 <h8_remote_free_publish_known+0x150>
    5443:	41 bc 04 00 00 00    	mov    $0x4,%r12d
    5449:	48 83 c4 08          	add    $0x8,%rsp
    544d:	44 89 e0             	mov    %r12d,%eax
    5450:	5b                   	pop    %rbx
    5451:	5d                   	pop    %rbp
    5452:	41 5c                	pop    %r12
    5454:	41 5d                	pop    %r13
    5456:	c3                   	ret    
    5457:	66 0f 1f 84 00 00 00 00 00 	nopw   0x0(%rax,%rax,1)
    5460:	41 0f b6 84 24 61 54 00 00 	movzbl 0x5461(%r12),%eax
    5469:	84 c0                	test   %al,%al
    546b:	0f 85 87 01 00 00    	jne    55f8 <h8_remote_free_publish_known+0x208>
    5471:	48 c1 ee 06          	shr    $0x6,%rsi
    5475:	4c 89 ef             	mov    %r13,%rdi
    5478:	0f b7 f6             	movzwl %si,%esi
    547b:	e8 60 f7 ff ff       	call   4be0 <h8_owner_publish_enter>
    5480:	84 c0                	test   %al,%al
    5482:	74 bf                	je     5443 <h8_remote_free_publish_known+0x53>
    5484:	48 8b 75 18          	mov    0x18(%rbp),%rsi
    5488:	41 0f b6 94 24 62 54 00 00 	movzbl 0x5462(%r12),%edx
    5491:	41 bc 02 00 00 00    	mov    $0x2,%r12d
    5497:	48 8b 45 20          	mov    0x20(%rbp),%rax
    549b:	8b 04 98             	mov    (%rax,%rbx,4),%eax
    549e:	c1 e8 1e             	shr    $0x1e,%eax
    54a1:	83 f8 01             	cmp    $0x1,%eax
    54a4:	75 7d                	jne    5523 <h8_remote_free_publish_known+0x133>
    54a6:	84 d2                	test   %dl,%dl
    54a8:	75 76                	jne    5520 <h8_remote_free_publish_known+0x130>
    54aa:	49 89 d8             	mov    %rbx,%r8
    54ad:	89 d9                	mov    %ebx,%ecx
    54af:	ba 01 00 00 00       	mov    $0x1,%edx
    54b4:	41 89 d9             	mov    %ebx,%r9d
    54b7:	49 c1 e8 06          	shr    $0x6,%r8
    54bb:	48 d3 e2             	shl    %cl,%rdx
    54be:	41 83 e1 3f          	and    $0x3f,%r9d
    54c2:	4a 8d 0c c6          	lea    (%rsi,%r8,8),%rcx
    54c6:	48 8b 01             	mov    (%rcx),%rax
    54c9:	48 89 c7             	mov    %rax,%rdi
    54cc:	48 89 c6             	mov    %rax,%rsi
    54cf:	48 09 d7             	or     %rdx,%rdi
    54d2:	f0 48 0f b1 39       	lock cmpxchg %rdi,(%rcx)
    54d7:	75 f0                	jne    54c9 <h8_remote_free_publish_known+0xd9>
    54d9:	41 bc 03 00 00 00    	mov    $0x3,%r12d
    54df:	48 85 f2             	test   %rsi,%rdx
    54e2:	75 3f                	jne    5523 <h8_remote_free_publish_known+0x133>
    54e4:	48 8b 45 20          	mov    0x20(%rbp),%rax
    54e8:	8b 04 98             	mov    (%rax,%rbx,4),%eax
    54eb:	c1 e8 1e             	shr    $0x1e,%eax
    54ee:	83 f8 01             	cmp    $0x1,%eax
    54f1:	0f 85 d9 01 00 00    	jne    56d0 <h8_remote_free_publish_known+0x2e0>
    54f7:	48 85 f6             	test   %rsi,%rsi
    54fa:	75 24                	jne    5520 <h8_remote_free_publish_known+0x130>
    54fc:	44 89 c1             	mov    %r8d,%ecx
    54ff:	b8 01 00 00 00       	mov    $0x1,%eax
    5504:	48 d3 e0             	shl    %cl,%rax
    5507:	f0 48 09 45 60       	lock or %rax,0x60(%rbp)
    550c:	48 89 ee             	mov    %rbp,%rsi
    550f:	4c 89 ef             	mov    %r13,%rdi
    5512:	e8 c9 fd ff ff       	call   52e0 <h8_span_notify>
    5517:	66 0f 1f 84 00 00 00 00 00 	nopw   0x0(%rax,%rax,1)
    5520:	45 31 e4             	xor    %r12d,%r12d
    5523:	4c 89 ef             	mov    %r13,%rdi
    5526:	e8 c5 f6 ff ff       	call   4bf0 <h8_owner_publish_exit>
    552b:	48 83 c4 08          	add    $0x8,%rsp
    552f:	44 89 e0             	mov    %r12d,%eax
    5532:	5b                   	pop    %rbx
    5533:	5d                   	pop    %rbp
    5534:	41 5c                	pop    %r12
    5536:	41 5d                	pop    %r13
    5538:	c3                   	ret    
    5539:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
    5540:	48 8b 75 18          	mov    0x18(%rbp),%rsi
    5544:	41 0f b6 94 24 62 54 00 00 	movzbl 0x5462(%r12),%edx
    554d:	41 bc 02 00 00 00    	mov    $0x2,%r12d
    5553:	48 8b 45 20          	mov    0x20(%rbp),%rax
    5557:	8b 04 98             	mov    (%rax,%rbx,4),%eax
    555a:	c1 e8 1e             	shr    $0x1e,%eax
    555d:	83 f8 01             	cmp    $0x1,%eax
    5560:	75 79                	jne    55db <h8_remote_free_publish_known+0x1eb>
    5562:	84 d2                	test   %dl,%dl
    5564:	75 72                	jne    55d8 <h8_remote_free_publish_known+0x1e8>
    5566:	49 89 d8             	mov    %rbx,%r8
    5569:	89 d9                	mov    %ebx,%ecx
    556b:	ba 01 00 00 00       	mov    $0x1,%edx
    5570:	41 89 d9             	mov    %ebx,%r9d
    5573:	49 c1 e8 06          	shr    $0x6,%r8
    5577:	48 d3 e2             	shl    %cl,%rdx
    557a:	41 83 e1 3f          	and    $0x3f,%r9d
    557e:	4a 8d 0c c6          	lea    (%rsi,%r8,8),%rcx
    5582:	48 8b 01             	mov    (%rcx),%rax
    5585:	48 89 c7             	mov    %rax,%rdi
    5588:	48 89 c6             	mov    %rax,%rsi
    558b:	48 09 d7             	or     %rdx,%rdi
    558e:	f0 48 0f b1 39       	lock cmpxchg %rdi,(%rcx)
    5593:	75 f0                	jne    5585 <h8_remote_free_publish_known+0x195>
    5595:	41 bc 03 00 00 00    	mov    $0x3,%r12d
    559b:	48 85 f2             	test   %rsi,%rdx
    559e:	75 3b                	jne    55db <h8_remote_free_publish_known+0x1eb>
    55a0:	48 8b 45 20          	mov    0x20(%rbp),%rax
    55a4:	8b 04 98             	mov    (%rax,%rbx,4),%eax
    55a7:	c1 e8 1e             	shr    $0x1e,%eax
    55aa:	83 f8 01             	cmp    $0x1,%eax
    55ad:	0f 85 f5 00 00 00    	jne    56a8 <h8_remote_free_publish_known+0x2b8>
    55b3:	48 85 f6             	test   %rsi,%rsi
    55b6:	75 20                	jne    55d8 <h8_remote_free_publish_known+0x1e8>
    55b8:	44 89 c1             	mov    %r8d,%ecx
    55bb:	b8 01 00 00 00       	mov    $0x1,%eax
    55c0:	48 d3 e0             	shl    %cl,%rax
    55c3:	f0 48 09 45 60       	lock or %rax,0x60(%rbp)
    55c8:	48 89 ee             	mov    %rbp,%rsi
    55cb:	4c 89 ef             	mov    %r13,%rdi
    55ce:	e8 0d fd ff ff       	call   52e0 <h8_span_notify>
    55d3:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
    55d8:	45 31 e4             	xor    %r12d,%r12d
    55db:	48 89 ef             	mov    %rbp,%rdi
    55de:	e8 8d ea ff ff       	call   4070 <h8_span_publish_exit>
    55e3:	48 83 c4 08          	add    $0x8,%rsp
    55e7:	44 89 e0             	mov    %r12d,%eax
    55ea:	5b                   	pop    %rbx
    55eb:	5d                   	pop    %rbp
    55ec:	41 5c                	pop    %r12
    55ee:	41 5d                	pop    %r13
    55f0:	c3                   	ret    
    55f1:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
    55f8:	48 8b 77 18          	mov    0x18(%rdi),%rsi
    55fc:	41 0f b6 94 24 62 54 00 00 	movzbl 0x5462(%r12),%edx
    5605:	41 bc 02 00 00 00    	mov    $0x2,%r12d
    560b:	48 8b 47 20          	mov    0x20(%rdi),%rax
    560f:	8b 04 98             	mov    (%rax,%rbx,4),%eax
    5612:	c1 e8 1e             	shr    $0x1e,%eax
    5615:	83 f8 01             	cmp    $0x1,%eax
    5618:	0f 85 2b fe ff ff    	jne    5449 <h8_remote_free_publish_known+0x59>
    561e:	84 d2                	test   %dl,%dl
    5620:	74 0e                	je     5630 <h8_remote_free_publish_known+0x240>
    5622:	45 31 e4             	xor    %r12d,%r12d
    5625:	e9 1f fe ff ff       	jmp    5449 <h8_remote_free_publish_known+0x59>
    562a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
    5630:	49 89 d8             	mov    %rbx,%r8
    5633:	89 d9                	mov    %ebx,%ecx
    5635:	ba 01 00 00 00       	mov    $0x1,%edx
    563a:	41 89 d9             	mov    %ebx,%r9d
    563d:	49 c1 e8 06          	shr    $0x6,%r8
    5641:	48 d3 e2             	shl    %cl,%rdx
    5644:	41 83 e1 3f          	and    $0x3f,%r9d
    5648:	4a 8d 0c c6          	lea    (%rsi,%r8,8),%rcx
    564c:	48 8b 01             	mov    (%rcx),%rax
    564f:	48 89 c7             	mov    %rax,%rdi
    5652:	48 89 c6             	mov    %rax,%rsi
    5655:	48 09 d7             	or     %rdx,%rdi
    5658:	f0 48 0f b1 39       	lock cmpxchg %rdi,(%rcx)
    565d:	75 f0                	jne    564f <h8_remote_free_publish_known+0x25f>
    565f:	41 bc 03 00 00 00    	mov    $0x3,%r12d
    5665:	48 85 f2             	test   %rsi,%rdx
    5668:	0f 85 db fd ff ff    	jne    5449 <h8_remote_free_publish_known+0x59>
    566e:	48 8b 45 20          	mov    0x20(%rbp),%rax
    5672:	8b 04 98             	mov    (%rax,%rbx,4),%eax
    5675:	c1 e8 1e             	shr    $0x1e,%eax
    5678:	83 f8 01             	cmp    $0x1,%eax
    567b:	0f 84 7f 00 00 00    	je     5700 <h8_remote_free_publish_known+0x310>
    5681:	0f b6 45 52          	movzbl 0x52(%rbp),%eax
    5685:	83 e8 02             	sub    $0x2,%eax
    5688:	3c 01                	cmp    $0x1,%al
    568a:	76 96                	jbe    5622 <h8_remote_free_publish_known+0x232>
    568c:	45 89 c9             	mov    %r9d,%r9d
    568f:	45 31 e4             	xor    %r12d,%r12d
    5692:	f0 4c 0f b3 09       	lock btr %r9,(%rcx)
    5697:	41 0f 92 c4          	setb   %r12b
    569b:	45 01 e4             	add    %r12d,%r12d
    569e:	e9 a6 fd ff ff       	jmp    5449 <h8_remote_free_publish_known+0x59>
    56a3:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
    56a8:	0f b6 45 52          	movzbl 0x52(%rbp),%eax
    56ac:	83 e8 02             	sub    $0x2,%eax
    56af:	3c 01                	cmp    $0x1,%al
    56b1:	0f 86 21 ff ff ff    	jbe    55d8 <h8_remote_free_publish_known+0x1e8>
    56b7:	45 89 c9             	mov    %r9d,%r9d
    56ba:	45 31 e4             	xor    %r12d,%r12d
    56bd:	f0 4c 0f b3 09       	lock btr %r9,(%rcx)
    56c2:	41 0f 92 c4          	setb   %r12b
    56c6:	45 01 e4             	add    %r12d,%r12d
    56c9:	e9 0d ff ff ff       	jmp    55db <h8_remote_free_publish_known+0x1eb>
    56ce:	66 90                	xchg   %ax,%ax
    56d0:	0f b6 45 52          	movzbl 0x52(%rbp),%eax
    56d4:	83 e8 02             	sub    $0x2,%eax
    56d7:	3c 01                	cmp    $0x1,%al
    56d9:	0f 86 41 fe ff ff    	jbe    5520 <h8_remote_free_publish_known+0x130>
    56df:	45 89 c9             	mov    %r9d,%r9d
    56e2:	45 31 e4             	xor    %r12d,%r12d
    56e5:	f0 4c 0f b3 09       	lock btr %r9,(%rcx)
    56ea:	41 0f 92 c4          	setb   %r12b
    56ee:	45 01 e4             	add    %r12d,%r12d
    56f1:	e9 2d fe ff ff       	jmp    5523 <h8_remote_free_publish_known+0x133>
    56f6:	66 2e 0f 1f 84 00 00 00 00 00 	cs nopw 0x0(%rax,%rax,1)
    5700:	48 85 f6             	test   %rsi,%rsi
    5703:	0f 85 19 ff ff ff    	jne    5622 <h8_remote_free_publish_known+0x232>
    5709:	44 89 c1             	mov    %r8d,%ecx
    570c:	b8 01 00 00 00       	mov    $0x1,%eax
    5711:	48 d3 e0             	shl    %cl,%rax
    5714:	f0 48 09 45 60       	lock or %rax,0x60(%rbp)
    5719:	48 89 ee             	mov    %rbp,%rsi
    571c:	4c 89 ef             	mov    %r13,%rdi
    571f:	e8 bc fb ff ff       	call   52e0 <h8_span_notify>
    5724:	e9 f9 fe ff ff       	jmp    5622 <h8_remote_free_publish_known+0x232>
    5729:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)

