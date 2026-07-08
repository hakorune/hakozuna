-- HZ11LaneFullEvidenceGate-L1: in-memory SQLite workload (real database).
-- Malloc-heavy: 100k row insert + index + scans, all in :memory: (no file I/O).
PRAGMA journal_mode=MEMORY;
CREATE TABLE t (id INTEGER PRIMARY KEY, v TEXT);
WITH RECURSIVE cnt(x) AS (SELECT 1 UNION ALL SELECT x+1 FROM cnt WHERE x<100000)
  INSERT INTO t(v) SELECT hex(randomblob(40)) FROM cnt;
SELECT count(*), sum(length(v)) FROM t;
SELECT * FROM t WHERE v LIKE 'AA%' LIMIT 200;
DELETE FROM t WHERE id % 2 = 0;
SELECT count(*) FROM t;
