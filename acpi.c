#include "acpi.h"
#include "utils.h"
#include "panic.h"

static struct acpi_rsdp* find_rsdp_in_region(void* start, size_t len) {
  for (size_t i = 0; i < len - 8; i++) {
    void* addr = (uint8_t*)start + i;
    if (memcmp("RSD PTR ", (const char*)addr, 8) == 0) {
      return (struct acpi_rsdp*)addr;
    }
  }
  return NULL;
}

uint32_t calc_checksum(uint8_t* addr, size_t size) {
  uint32_t checksum = 0;
  for (size_t i = 0; i < size; ++i) {
    checksum += addr[i];
  }
  return checksum;
}

void acpi_verify_rsdp(struct acpi_rsdp* rsdp) {
  uint8_t* addr = (uint8_t*)rsdp;
  uint32_t checksum = 0;
  for (int i = 0; i < (int)sizeof(*rsdp); ++i) {
    checksum += addr[i];
  }
  printf_("RSDP checksum: 0x%x", checksum);
  if (checksum % 0x100 != 0) {
    panic("RSDP checksum invalid");
  } else {
    printf_(": OK\n");
  }
}

void acpi_verify_sdt(struct acpi_sdt* sdt) {
  struct acpi_sdt_header* header = &sdt->header;

  uint32_t header_size = header->length;
  uint32_t checksum = calc_checksum(header, header_size);

  char sign[5];
  memcpy(sign, header->signature, 4);
  sign[4] = '\0';

  printf_("%s checksum: 0x%x", sign, checksum);
  if (checksum % 0x100 != 0) {
    panic("SDT checksum invalid");
  } else {
    printf_(": OK\n");
  }

  if (!memcmp(header->signature, "RSDT", 4)) {
    size_t entries_cnt = (sdt->header.length - sizeof(sdt->header)) / 4;
    for (size_t i = 0; i < entries_cnt; ++i) {
      acpi_verify_sdt(sdt->entries[i]);
    }
  }
}

struct acpi_sdt* acpi_find_rsdt() {
  // 1KB of EBDA.
  void* ebda_addr = (void*)((*(uint16_t*)0x40e) << 4);
  struct acpi_rsdp* rsdp = find_rsdp_in_region(ebda_addr, 1024);
  if (!rsdp) {
    // Static memory region.
    rsdp = find_rsdp_in_region((void*)0xe0000, 0xfffff - 0xe0000);
  }

  if (!rsdp) {
    return NULL;
  }

  if (rsdp->revision != 0) {
    panic("APIC Version > 1.0 not supported\n");
  }

  acpi_verify_rsdp(rsdp);

  struct acpi_sdt* rsdt_addr = (struct acpi_sdt*)rsdp->rsdt_addr;
  acpi_verify_sdt(rsdt_addr);

  return rsdt_addr;
}

struct acpi_sdt* acpi_find_sdt(struct acpi_sdt* root, const char* signature) {
  size_t sz = (root->header.length - sizeof(root->header)) / 4;
  for (size_t i = 0; i < sz; i++) {
    if (memcmp(signature, &root->entries[i]->header.signature, 4) == 0) {
      return root->entries[i];
    }
  }
  return NULL;
}
