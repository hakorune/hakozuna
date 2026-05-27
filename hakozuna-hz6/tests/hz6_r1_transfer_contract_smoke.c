#include "../include/hz6_contract.h"
#include "../transfer/hz6_transfer.h"
#include "../transfer/hz6_transfer_backend.h"

#include <stdio.h>

typedef struct SmokeDescriptor {
  int marker;
} SmokeDescriptor;

static int expect(int condition, const char* label) {
  if (!condition) {
    fprintf(stderr, "hz6-r1-transfer-contract-smoke failed: %s\n", label);
    return 0;
  }
  return 1;
}

int main(void) {
  SmokeDescriptor descriptor;
  descriptor.marker = 42;

  unsigned char object[256];
  void* base = (void*)object;

  Hz6TransferObject transfer_objects[2];
  Hz6TransferCache transfer;
  hz6_transfer_init(&transfer, transfer_objects, 2);

  Hz6TransferObject object_a;
  object_a.ptr = base;
  object_a.descriptor = &descriptor;
  object_a.class_id = 7;
  object_a.generation = 11;

  Hz6TransferObject object_b = object_a;
  object_b.ptr = object + 128;
  object_b.class_id = 8;
  Hz6TransferObject object_c = object_a;
  object_c.ptr = object + 32;
  object_c.class_id = 7;
  Hz6TransferObject object_d = object_a;
  object_d.ptr = object + 64;
  object_d.class_id = 7;
  Hz6TransferObject object_e = object_a;
  object_e.ptr = object + 96;
  object_e.class_id = 7;

  if (!expect(hz6_transfer_push(&transfer, object_a), "transfer push a") ||
      !expect(hz6_transfer_push(&transfer, object_b), "transfer push b") ||
      !expect(hz6_transfer_count_class(&transfer, 7) == 1,
              "transfer class count a") ||
      !expect(hz6_transfer_count_class(&transfer, 8) == 1,
              "transfer class count b") ||
      !expect(!hz6_transfer_push(&transfer, object_a), "transfer bounded")) {
    return 1;
  }

  Hz6TransferObject popped;
  if (!expect(hz6_transfer_pop(&transfer, 7, &popped),
              "transfer pop class") ||
      !expect(popped.ptr == base, "transfer pop pointer") ||
      !expect(hz6_transfer_count(&transfer) == 1, "transfer count")) {
    return 1;
  }

  Hz6TransferObject backend_objects[2];
  Hz6TransferBackend transfer_backend;
  hz6_transfer_backend_init_single(&transfer_backend, backend_objects, 2);
  if (!expect(hz6_transfer_backend_push(&transfer_backend, object_a),
              "transfer backend push a") ||
      !expect(hz6_transfer_backend_push(&transfer_backend, object_b),
              "transfer backend push b") ||
      !expect(hz6_transfer_backend_capacity(&transfer_backend) == 2,
              "transfer backend capacity") ||
      !expect(hz6_transfer_backend_count_class(&transfer_backend, 7) == 1,
              "transfer backend class count a") ||
      !expect(!hz6_transfer_backend_push(&transfer_backend, object_a),
              "transfer backend bounded")) {
    return 1;
  }
  Hz6TransferObject backend_popped;
  if (!expect(hz6_transfer_backend_pop(&transfer_backend, 8,
                                       &backend_popped),
              "transfer backend pop class") ||
      !expect(backend_popped.ptr == object_b.ptr,
              "transfer backend pop pointer") ||
      !expect(hz6_transfer_backend_count(&transfer_backend) == 1,
              "transfer backend count")) {
    return 1;
  }

  Hz6TransferObject sharded_objects[4];
  Hz6TransferBackend sharded_backend;
  hz6_transfer_backend_init_sharded(&sharded_backend, sharded_objects, 4, 2);
  if (!expect(sharded_backend.kind == HZ6_TRANSFER_BACKEND_SHARDED_CACHE,
              "transfer sharded kind") ||
      !expect(hz6_transfer_backend_push(&sharded_backend, object_a),
              "transfer sharded push a") ||
      !expect(hz6_transfer_backend_push(&sharded_backend, object_b),
              "transfer sharded push b") ||
      !expect(hz6_transfer_backend_capacity(&sharded_backend) == 4,
              "transfer sharded capacity") ||
      !expect(hz6_transfer_backend_shard_count_at(&sharded_backend, 0) == 1,
              "transfer sharded shard zero count") ||
      !expect(hz6_transfer_backend_shard_count_at(&sharded_backend, 1) == 1,
              "transfer sharded shard one count") ||
      !expect(hz6_transfer_backend_count_class(&sharded_backend, 7) == 1,
              "transfer sharded class count") ||
      !expect(hz6_transfer_backend_count(&sharded_backend) == 2,
              "transfer sharded count")) {
    return 1;
  }
  Hz6TransferObject sharded_popped;
  if (!expect(hz6_transfer_backend_pop(&sharded_backend, 7,
                                       &sharded_popped),
              "transfer sharded pop class") ||
      !expect(sharded_popped.ptr == object_a.ptr,
              "transfer sharded pop pointer") ||
      !expect(hz6_transfer_backend_count(&sharded_backend) == 1,
              "transfer sharded count after pop") ||
      !expect(hz6_transfer_backend_count_class(&sharded_backend, 8) == 1,
              "transfer sharded other class retained")) {
    return 1;
  }
  Hz6TransferObject sharded_other_popped;
  if (!expect(hz6_transfer_backend_pop(&sharded_backend, 8,
                                       &sharded_other_popped),
              "transfer sharded other class pop") ||
      !expect(sharded_other_popped.ptr == object_b.ptr,
              "transfer sharded other class pointer") ||
      !expect(hz6_transfer_backend_count(&sharded_backend) == 0,
              "transfer sharded count after pop")) {
    return 1;
  }

  Hz6TransferObject steal_objects[4];
  Hz6TransferBackend steal_backend;
  hz6_transfer_backend_init_sharded(&steal_backend, steal_objects, 4, 2);
  if (!expect(hz6_transfer_backend_push(&steal_backend, object_a),
              "transfer steal push") ||
      !expect(hz6_transfer_backend_shard_count_at(&steal_backend, 0) == 1,
              "transfer steal producer shard") ||
      !expect(hz6_transfer_backend_shard_count_at(&steal_backend, 1) == 0,
              "transfer steal empty home shard")) {
    return 1;
  }
  Hz6TransferObject steal_popped;
  if (!expect(hz6_transfer_backend_pop(&steal_backend, 7, &steal_popped),
              "transfer steal pop from non-home shard") ||
      !expect(steal_popped.ptr == object_a.ptr,
              "transfer steal pop pointer") ||
      !expect(hz6_transfer_backend_count(&steal_backend) == 0,
              "transfer steal empty after pop")) {
    return 1;
  }

  Hz6TransferObject home_objects[4];
  Hz6TransferBackend home_backend;
  hz6_transfer_backend_init_sharded(&home_backend, home_objects, 4, 2);
  Hz6TransferObject object_home0 = object_a;
  object_home0.ptr = object + 16;
  object_home0.class_id = 10;
  Hz6TransferObject object_home1 = object_a;
  object_home1.ptr = object + 48;
  object_home1.class_id = 10;
  if (!expect(hz6_transfer_backend_push(&home_backend, object_home0),
              "transfer home push first") ||
      !expect(hz6_transfer_backend_push(&home_backend, object_home1),
              "transfer home push second") ||
      !expect(hz6_transfer_backend_shard_count_at(&home_backend, 0) == 1,
              "transfer home shard zero seeded") ||
      !expect(hz6_transfer_backend_shard_count_at(&home_backend, 1) == 1,
              "transfer home shard one seeded")) {
    return 1;
  }
  Hz6TransferObject home_popped;
  if (!expect(hz6_transfer_backend_pop_from_shard(&home_backend, 10, 1,
                                                  &home_popped),
              "transfer explicit home pop") ||
      !expect(home_popped.ptr == object_home1.ptr,
              "transfer explicit home pointer") ||
      !expect(hz6_transfer_backend_pop_from_shard(&home_backend, 10, 1,
                                                  &home_popped),
              "transfer explicit home steal") ||
      !expect(home_popped.ptr == object_home0.ptr,
              "transfer explicit home steal pointer") ||
      !expect(hz6_transfer_backend_count(&home_backend) == 0,
              "transfer explicit home empty")) {
    return 1;
  }

  Hz6TransferObject push_home_objects[4];
  Hz6TransferBackend push_home_backend;
  hz6_transfer_backend_init_sharded(&push_home_backend, push_home_objects, 4,
                                    2);
  if (!expect(hz6_transfer_backend_push_to_shard(&push_home_backend,
                                                 object_home0, 1),
              "transfer explicit push shard one") ||
      !expect(hz6_transfer_backend_shard_count_at(&push_home_backend, 0) == 0,
              "transfer explicit push keeps shard zero empty") ||
      !expect(hz6_transfer_backend_shard_count_at(&push_home_backend, 1) == 1,
              "transfer explicit push shard one count") ||
      !expect(hz6_transfer_backend_push_to_shard(&push_home_backend,
                                                 object_home1, 1),
              "transfer explicit push shard one second") ||
      !expect(hz6_transfer_backend_push_to_shard(&push_home_backend,
                                                 object_home0, 1),
              "transfer explicit push fallback shard zero") ||
      !expect(hz6_transfer_backend_shard_count_at(&push_home_backend, 0) == 1,
              "transfer explicit push fallback count") ||
      !expect(hz6_transfer_backend_shard_count_at(&push_home_backend, 1) == 2,
              "transfer explicit push filled home count")) {
    return 1;
  }

  Hz6TransferObject uneven_objects[5];
  Hz6TransferBackend uneven_backend;
  hz6_transfer_backend_init_sharded(&uneven_backend, uneven_objects, 5, 2);
  if (!expect(uneven_backend.kind == HZ6_TRANSFER_BACKEND_SHARDED_CACHE,
              "transfer uneven sharded kind") ||
      !expect(hz6_transfer_backend_capacity(&uneven_backend) == 5,
              "transfer uneven sharded capacity") ||
      !expect(hz6_transfer_backend_shard_capacity_at(&uneven_backend, 0) == 3,
              "transfer uneven shard zero capacity") ||
      !expect(hz6_transfer_backend_shard_capacity_at(&uneven_backend, 1) == 2,
              "transfer uneven shard one capacity") ||
      !expect(hz6_transfer_backend_shard_capacity_at(&uneven_backend, 2) == 0,
              "transfer uneven inactive shard capacity")) {
    return 1;
  }
  if (!expect(hz6_transfer_backend_push(&uneven_backend, object_a),
              "transfer uneven push a") ||
      !expect(hz6_transfer_backend_push(&uneven_backend, object_b),
              "transfer uneven push b") ||
      !expect(hz6_transfer_backend_push(&uneven_backend, object_c),
              "transfer uneven push c") ||
      !expect(hz6_transfer_backend_push(&uneven_backend, object_d),
              "transfer uneven push d") ||
      !expect(hz6_transfer_backend_push(&uneven_backend, object_e),
              "transfer uneven push e") ||
      !expect(hz6_transfer_backend_count(&uneven_backend) == 5,
              "transfer uneven full count") ||
      !expect(hz6_transfer_backend_shard_count_at(&uneven_backend, 0) == 3,
              "transfer uneven shard zero full") ||
      !expect(hz6_transfer_backend_shard_count_at(&uneven_backend, 1) == 2,
              "transfer uneven shard one full") ||
      !expect(!hz6_transfer_backend_push(&uneven_backend, object_a),
              "transfer uneven bounded after full")) {
    return 1;
  }

  printf("hz6-r1-transfer-contract-smoke ok\n");
  return 0;
}
