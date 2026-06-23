0000000000005e70 <h8_malloc_inner>:
    5e70:	f3 0f 1e fa          	endbr64 
    5e74:	41 55                	push   %r13
    5e76:	41 54                	push   %r12
    5e78:	55                   	push   %rbp
    5e79:	53                   	push   %rbx
    5e7a:	48 83 ec 18          	sub    $0x18,%rsp
    5e7e:	48 85 ff             	test   %rdi,%rdi
    5e81:	0f 84 99 00 00 00    	je     5f20 <h8_malloc_inner+0xb0>
    5e87:	48 81 ff 00 10 00 00 	cmp    $0x1000,%rdi
    5e8e:	0f 87 8c 01 00 00    	ja     6020 <h8_malloc_inner+0x1b0>
    5e94:	48 c7 c0 f8 ff ff ff 	mov    $0xfffffffffffffff8,%rax
    5e9b:	64 48 8b 18          	mov    %fs:(%rax),%rbx
    5e9f:	48 85 db             	test   %rbx,%rbx
    5ea2:	0f 84 08 02 00 00    	je     60b0 <h8_malloc_inner+0x240>
    5ea8:	48 83 ff 10          	cmp    $0x10,%rdi
    5eac:	0f 86 4e 02 00 00    	jbe    6100 <h8_malloc_inner+0x290>
    5eb2:	48 83 ef 01          	sub    $0x1,%rdi
    5eb6:	bd 3c 00 00 00       	mov    $0x3c,%ebp
    5ebb:	48 0f bd ff          	bsr    %rdi,%rdi
    5ebf:	48 83 f7 3f          	xor    $0x3f,%rdi
    5ec3:	29 fd                	sub    %edi,%ebp
    5ec5:	41 89 ec             	mov    %ebp,%r12d
    5ec8:	4a 8b 54 e3 08       	mov    0x8(%rbx,%r12,8),%rdx
    5ecd:	48 85 d2             	test   %rdx,%rdx
    5ed0:	0f 84 a2 00 00 00    	je     5f78 <h8_malloc_inner+0x108>
    5ed6:	8b 82 84 00 00 00    	mov    0x84(%rdx),%eax
    5edc:	3d ff ff ff 3f       	cmp    $0x3fffffff,%eax
    5ee1:	74 6d                	je     5f50 <h8_malloc_inner+0xe0>
    5ee3:	48 8b 4a 20          	mov    0x20(%rdx),%rcx
    5ee7:	48 8d 34 81          	lea    (%rcx,%rax,4),%rsi
    5eeb:	8b 0e                	mov    (%rsi),%ecx
    5eed:	81 e1 ff ff ff 3f    	and    $0x3fffffff,%ecx
    5ef3:	89 8a 84 00 00 00    	mov    %ecx,0x84(%rdx)
    5ef9:	c7 06 00 00 00 40    	movl   $0x40000000,(%rsi)
    5eff:	0f b7 4a 08          	movzwl 0x8(%rdx),%ecx
    5f03:	83 c1 04             	add    $0x4,%ecx
    5f06:	48 d3 e0             	shl    %cl,%rax
    5f09:	48 03 02             	add    (%rdx),%rax
    5f0c:	48 85 c0             	test   %rax,%rax
    5f0f:	74 51                	je     5f62 <h8_malloc_inner+0xf2>
    5f11:	48 83 c4 18          	add    $0x18,%rsp
    5f15:	5b                   	pop    %rbx
    5f16:	5d                   	pop    %rbp
    5f17:	41 5c                	pop    %r12
    5f19:	41 5d                	pop    %r13
    5f1b:	c3                   	ret    
    5f1c:	0f 1f 40 00          	nopl   0x0(%rax)
    5f20:	48 c7 c0 f8 ff ff ff 	mov    $0xfffffffffffffff8,%rax
    5f27:	31 ed                	xor    %ebp,%ebp
    5f29:	64 48 8b 18          	mov    %fs:(%rax),%rbx
    5f2d:	48 85 db             	test   %rbx,%rbx
    5f30:	75 93                	jne    5ec5 <h8_malloc_inner+0x55>
    5f32:	e8 d9 dc ff ff       	call   3c10 <h8_thread_ctx_get_slow>
    5f37:	48 89 c3             	mov    %rax,%rbx
    5f3a:	48 85 c0             	test   %rax,%rax
    5f3d:	75 86                	jne    5ec5 <h8_malloc_inner+0x55>
    5f3f:	90                   	nop
    5f40:	31 c0                	xor    %eax,%eax
    5f42:	48 83 c4 18          	add    $0x18,%rsp
    5f46:	5b                   	pop    %rbx
    5f47:	5d                   	pop    %rbp
    5f48:	41 5c                	pop    %r12
    5f4a:	41 5d                	pop    %r13
    5f4c:	c3                   	ret    
    5f4d:	0f 1f 00             	nopl   (%rax)
    5f50:	8b 82 80 00 00 00    	mov    0x80(%rdx),%eax
    5f56:	0f b7 4a 0a          	movzwl 0xa(%rdx),%ecx
    5f5a:	39 c8                	cmp    %ecx,%eax
    5f5c:	0f 82 ce 00 00 00    	jb     6030 <h8_malloc_inner+0x1c0>
    5f62:	4c 8b 2b             	mov    (%rbx),%r13
    5f65:	4c 89 ef             	mov    %r13,%rdi
    5f68:	e8 63 f2 ff ff       	call   51d0 <h8_pressure_owner_collect>
    5f6d:	4d 85 ed             	test   %r13,%r13
    5f70:	75 09                	jne    5f7b <h8_malloc_inner+0x10b>
    5f72:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
    5f78:	4c 8b 2b             	mov    (%rbx),%r13
    5f7b:	89 ea                	mov    %ebp,%edx
    5f7d:	4c 89 ee             	mov    %r13,%rsi
    5f80:	48 89 df             	mov    %rbx,%rdi
    5f83:	e8 c8 fd ff ff       	call   5d50 <h8_find_active_span>
    5f88:	48 89 c2             	mov    %rax,%rdx
    5f8b:	48 85 c0             	test   %rax,%rax
    5f8e:	0f 84 cc 00 00 00    	je     6060 <h8_malloc_inner+0x1f0>
    5f94:	4a 89 54 e3 08       	mov    %rdx,0x8(%rbx,%r12,8)
    5f99:	8b 82 84 00 00 00    	mov    0x84(%rdx),%eax
    5f9f:	3d ff ff ff 3f       	cmp    $0x3fffffff,%eax
    5fa4:	74 3a                	je     5fe0 <h8_malloc_inner+0x170>
    5fa6:	48 8b 4a 20          	mov    0x20(%rdx),%rcx
    5faa:	48 8d 34 81          	lea    (%rcx,%rax,4),%rsi
    5fae:	8b 0e                	mov    (%rsi),%ecx
    5fb0:	81 e1 ff ff ff 3f    	and    $0x3fffffff,%ecx
    5fb6:	89 8a 84 00 00 00    	mov    %ecx,0x84(%rdx)
    5fbc:	c7 06 00 00 00 40    	movl   $0x40000000,(%rsi)
    5fc2:	0f b7 4a 08          	movzwl 0x8(%rdx),%ecx
    5fc6:	83 c1 04             	add    $0x4,%ecx
    5fc9:	48 d3 e0             	shl    %cl,%rax
    5fcc:	48 03 02             	add    (%rdx),%rax
    5fcf:	48 83 c4 18          	add    $0x18,%rsp
    5fd3:	5b                   	pop    %rbx
    5fd4:	5d                   	pop    %rbp
    5fd5:	41 5c                	pop    %r12
    5fd7:	41 5d                	pop    %r13
    5fd9:	c3                   	ret    
    5fda:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
    5fe0:	8b 82 80 00 00 00    	mov    0x80(%rdx),%eax
    5fe6:	0f b7 4a 0a          	movzwl 0xa(%rdx),%ecx
    5fea:	39 c8                	cmp    %ecx,%eax
    5fec:	0f 83 4e ff ff ff    	jae    5f40 <h8_malloc_inner+0xd0>
    5ff2:	8d 48 01             	lea    0x1(%rax),%ecx
    5ff5:	89 8a 80 00 00 00    	mov    %ecx,0x80(%rdx)
    5ffb:	48 8b 4a 20          	mov    0x20(%rdx),%rcx
    5fff:	c7 04 81 00 00 00 40 	movl   $0x40000000,(%rcx,%rax,4)
    6006:	0f b7 4a 08          	movzwl 0x8(%rdx),%ecx
    600a:	83 c1 04             	add    $0x4,%ecx
    600d:	48 d3 e0             	shl    %cl,%rax
    6010:	48 03 02             	add    (%rdx),%rax
    6013:	e9 f9 fe ff ff       	jmp    5f11 <h8_malloc_inner+0xa1>
    6018:	0f 1f 84 00 00 00 00 00 	nopl   0x0(%rax,%rax,1)
    6020:	48 83 c4 18          	add    $0x18,%rsp
    6024:	5b                   	pop    %rbx
    6025:	5d                   	pop    %rbp
    6026:	41 5c                	pop    %r12
    6028:	41 5d                	pop    %r13
    602a:	e9 91 02 00 00       	jmp    62c0 <h8_sys_malloc>
    602f:	90                   	nop
    6030:	8d 48 01             	lea    0x1(%rax),%ecx
    6033:	89 8a 80 00 00 00    	mov    %ecx,0x80(%rdx)
    6039:	48 8b 4a 20          	mov    0x20(%rdx),%rcx
    603d:	c7 04 81 00 00 00 40 	movl   $0x40000000,(%rcx,%rax,4)
    6044:	0f b7 4a 08          	movzwl 0x8(%rdx),%ecx
    6048:	83 c1 04             	add    $0x4,%ecx
    604b:	48 d3 e0             	shl    %cl,%rax
    604e:	48 03 02             	add    (%rdx),%rax
    6051:	e9 b6 fe ff ff       	jmp    5f0c <h8_malloc_inner+0x9c>
    6056:	66 2e 0f 1f 84 00 00 00 00 00 	cs nopw 0x0(%rax,%rax,1)
    6060:	4c 89 ef             	mov    %r13,%rdi
    6063:	e8 68 f1 ff ff       	call   51d0 <h8_pressure_owner_collect>
    6068:	89 ea                	mov    %ebp,%edx
    606a:	4c 89 ee             	mov    %r13,%rsi
    606d:	48 89 df             	mov    %rbx,%rdi
    6070:	e8 db fc ff ff       	call   5d50 <h8_find_active_span>
    6075:	48 89 c2             	mov    %rax,%rdx
    6078:	48 85 c0             	test   %rax,%rax
    607b:	0f 85 13 ff ff ff    	jne    5f94 <h8_malloc_inner+0x124>
    6081:	48 8d 05 f8 5f 00 00 	lea    0x5ff8(%rip),%rax        # c080 <h8g>
    6088:	0f b6 90 60 54 00 00 	movzbl 0x5460(%rax),%edx
    608f:	84 d2                	test   %dl,%dl
    6091:	75 45                	jne    60d8 <h8_malloc_inner+0x268>
    6093:	89 ee                	mov    %ebp,%esi
    6095:	4c 89 ef             	mov    %r13,%rdi
    6098:	e8 c3 d2 ff ff       	call   3360 <h8_span_commit_for_class>
    609d:	48 89 c2             	mov    %rax,%rdx
    60a0:	48 85 c0             	test   %rax,%rax
    60a3:	0f 85 eb fe ff ff    	jne    5f94 <h8_malloc_inner+0x124>
    60a9:	31 c0                	xor    %eax,%eax
    60ab:	e9 92 fe ff ff       	jmp    5f42 <h8_malloc_inner+0xd2>
    60b0:	48 89 7c 24 08       	mov    %rdi,0x8(%rsp)
    60b5:	e8 56 db ff ff       	call   3c10 <h8_thread_ctx_get_slow>
    60ba:	48 8b 7c 24 08       	mov    0x8(%rsp),%rdi
    60bf:	48 85 c0             	test   %rax,%rax
    60c2:	48 89 c3             	mov    %rax,%rbx
    60c5:	0f 85 dd fd ff ff    	jne    5ea8 <h8_malloc_inner+0x38>
    60cb:	31 c0                	xor    %eax,%eax
    60cd:	e9 70 fe ff ff       	jmp    5f42 <h8_malloc_inner+0xd2>
    60d2:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
    60d8:	48 8b 80 88 4e 00 00 	mov    0x4e88(%rax),%rax
    60df:	48 85 c0             	test   %rax,%rax
    60e2:	74 af                	je     6093 <h8_malloc_inner+0x223>
    60e4:	89 ee                	mov    %ebp,%esi
    60e6:	4c 89 ef             	mov    %r13,%rdi
    60e9:	e8 b2 e3 ff ff       	call   44a0 <h8_orphan_adopt_span>
    60ee:	48 89 c2             	mov    %rax,%rdx
    60f1:	48 85 c0             	test   %rax,%rax
    60f4:	74 9d                	je     6093 <h8_malloc_inner+0x223>
    60f6:	e9 99 fe ff ff       	jmp    5f94 <h8_malloc_inner+0x124>
    60fb:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
    6100:	31 ed                	xor    %ebp,%ebp
    6102:	e9 be fd ff ff       	jmp    5ec5 <h8_malloc_inner+0x55>
    6107:	66 0f 1f 84 00 00 00 00 00 	nopw   0x0(%rax,%rax,1)

