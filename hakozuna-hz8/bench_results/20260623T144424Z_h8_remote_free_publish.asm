0000000000005730 <h8_remote_free_publish>:
    5730:	f3 0f 1e fa          	endbr64 
    5734:	48 85 ff             	test   %rdi,%rdi
    5737:	74 1c                	je     5755 <h8_remote_free_publish+0x25>
    5739:	48 8d 05 40 69 00 00 	lea    0x6940(%rip),%rax        # c080 <h8g>
    5740:	48 8b 50 10          	mov    0x10(%rax),%rdx
    5744:	48 39 fa             	cmp    %rdi,%rdx
    5747:	77 0c                	ja     5755 <h8_remote_free_publish+0x25>
    5749:	48 8b 48 18          	mov    0x18(%rax),%rcx
    574d:	48 01 d1             	add    %rdx,%rcx
    5750:	48 39 cf             	cmp    %rcx,%rdi
    5753:	72 0b                	jb     5760 <h8_remote_free_publish+0x30>
    5755:	b8 01 00 00 00       	mov    $0x1,%eax
    575a:	c3                   	ret    
    575b:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
    5760:	48 89 f9             	mov    %rdi,%rcx
    5763:	48 8b 40 38          	mov    0x38(%rax),%rax
    5767:	48 29 d1             	sub    %rdx,%rcx
    576a:	48 89 ca             	mov    %rcx,%rdx
    576d:	48 c1 ea 10          	shr    $0x10,%rdx
    5771:	4c 8b 04 d0          	mov    (%rax,%rdx,8),%r8
    5775:	4d 85 c0             	test   %r8,%r8
    5778:	74 db                	je     5755 <h8_remote_free_publish+0x25>
    577a:	41 0f b6 40 4e       	movzbl 0x4e(%r8),%eax
    577f:	3c 05                	cmp    $0x5,%al
    5781:	74 d2                	je     5755 <h8_remote_free_publish+0x25>
    5783:	41 0f b7 48 08       	movzwl 0x8(%r8),%ecx
    5788:	48 c7 c0 ff ff ff ff 	mov    $0xffffffffffffffff,%rax
    578f:	49 2b 38             	sub    (%r8),%rdi
    5792:	83 c1 04             	add    $0x4,%ecx
    5795:	48 d3 e0             	shl    %cl,%rax
    5798:	48 f7 d0             	not    %rax
    579b:	48 85 f8             	test   %rdi,%rax
    579e:	75 b5                	jne    5755 <h8_remote_free_publish+0x25>
    57a0:	48 89 fe             	mov    %rdi,%rsi
    57a3:	41 0f b7 40 0a       	movzwl 0xa(%r8),%eax
    57a8:	48 d3 ee             	shr    %cl,%rsi
    57ab:	48 39 c6             	cmp    %rax,%rsi
    57ae:	73 a5                	jae    5755 <h8_remote_free_publish+0x25>
    57b0:	4c 89 c7             	mov    %r8,%rdi
    57b3:	e9 38 fc ff ff       	jmp    53f0 <h8_remote_free_publish_known>
    57b8:	0f 1f 84 00 00 00 00 00 	nopl   0x0(%rax,%rax,1)

