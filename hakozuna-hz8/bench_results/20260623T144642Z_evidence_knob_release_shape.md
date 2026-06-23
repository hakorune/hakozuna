# EvidenceKnobReleaseShape-L1 20260623T144642Z

HEAD before box: f664e8f

Scope:

- compile unsafe evidence knobs out of normal release hot paths
- keep explicit evidence builds possible with H8_ENABLE_UNSAFE_EVIDENCE_KNOBS
- no remote protocol authority change in normal release

Implementation:

- h8_remote_lease_elision_enabled() returns false unless H8_ENABLE_UNSAFE_EVIDENCE_KNOBS is defined
- h8_remote_pending_publish_elision_enabled() returns false unless H8_ENABLE_UNSAFE_EVIDENCE_KNOBS is defined
- unsafe evidence env parsing is compiled only in evidence builds
- HZ8_ENV_FLAGS documents that unsafe evidence flags are not read in normal release/debug/preload/bench targets

Validation:

- smoke: pass
- safety stress: pass
- preload smoke: pass
- explicit evidence macro build: pass
- normal bench-release rebuild after evidence macro check: pass
- interleaved remote90 R5 median: 56.05M ops/s
- zero gates in release snapshot: clean

Code shape:

- h8_remote_free_publish_known normal-release asm has no refs to remote_lease_elision_enabled or remote_pending_publish_elision_enabled
- h8_remote_free_publish_known no longer carries unsafe lease-elision branch
- h8_remote_free_publish_locked no longer carries unsafe drop-pending-publish branch in normal release

Saved data:

- bench_results/20260623T144642Z_evidence_knob_remote_known.asm
- bench_results/20260623T144642Z_evidence_knob_objdump.txt
- bench_results/20260623T144642Z_evidence_knob_interleaved_r5.log
- bench_results/20260623T144642Z_evidence_knob_smoke.log
- bench_results/20260623T144642Z_evidence_knob_safety.log
