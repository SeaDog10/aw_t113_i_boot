#include "boot.h"

#define HGBOOT_NULL     0

#if BOOT_LOG_LEVEL > BOOT_LOG_NONE
#include "shell/shell.h"
#endif

#if BOOT_LOG_LEVEL >= BOOT_LOG_TRACE
#define BOOT_TRACE(format, ...) do{s_printf("[T]");s_printf(format, ##__VA_ARGS__);}while (0)
#else
#define BOOT_TRACE(format, ...)
#endif

#if BOOT_LOG_LEVEL >= BOOT_LOG_DEBUG
#define BOOT_DEBUG(format, ...) do{s_printf("[D]");s_printf(format, ##__VA_ARGS__);}while (0)
#else
#define BOOT_DEBUG(format, ...)
#endif

#if BOOT_LOG_LEVEL >= BOOT_LOG_INFO
#define BOOT_INFO(format, ...) do{s_printf("[I]");s_printf(format, ##__VA_ARGS__);}while (0)
#else
#define BOOT_INFO(format, ...)
#endif

#if BOOT_LOG_LEVEL >= BOOT_LOG_WARN
#define BOOT_WARN(format, ...) do{s_printf("[W]");s_printf(format, ##__VA_ARGS__);}while (0)
#else
#define BOOT_WARN(format, ...)
#endif

#if BOOT_LOG_LEVEL >= BOOT_LOG_ERROR
#define BOOT_ERR(format, ...) do{s_printf("[E]");s_printf(format, ##__VA_ARGS__);}while (0)
#else
#define BOOT_ERR(format, ...)
#endif

static const unsigned int boot_crc32_table[256] =
{
    0x00000000U, 0x77073096U, 0xEE0E612CU, 0x990951BAU, 0x076DC419U, 0x706AF48FU, 0xE963A535U, 0x9E6495A3U,
    0x0EDB8832U, 0x79DCB8A4U, 0xE0D5E91EU, 0x97D2D988U, 0x09B64C2BU, 0x7EB17CBDU, 0xE7B82D07U, 0x90BF1D91U,
    0x1DB71064U, 0x6AB020F2U, 0xF3B97148U, 0x84BE41DEU, 0x1ADAD47DU, 0x6DDDE4EBU, 0xF4D4B551U, 0x83D385C7U,
    0x136C9856U, 0x646BA8C0U, 0xFD62F97AU, 0x8A65C9ECU, 0x14015C4FU, 0x63066CD9U, 0xFA0F3D63U, 0x8D080DF5U,
    0x3B6E20C8U, 0x4C69105EU, 0xD56041E4U, 0xA2677172U, 0x3C03E4D1U, 0x4B04D447U, 0xD20D85FDU, 0xA50AB56BU,
    0x35B5A8FAU, 0x42B2986CU, 0xDBBBC9D6U, 0xACBCF940U, 0x32D86CE3U, 0x45DF5C75U, 0xDCD60DCFU, 0xABD13D59U,
    0x26D930ACU, 0x51DE003AU, 0xC8D75180U, 0xBFD06116U, 0x21B4F4B5U, 0x56B3C423U, 0xCFBA9599U, 0xB8BDA50FU,
    0x2802B89EU, 0x5F058808U, 0xC60CD9B2U, 0xB10BE924U, 0x2F6F7C87U, 0x58684C11U, 0xC1611DABU, 0xB6662D3DU,
    0x76DC4190U, 0x01DB7106U, 0x98D220BCU, 0xEFD5102AU, 0x71B18589U, 0x06B6B51FU, 0x9FBFE4A5U, 0xE8B8D433U,
    0x7807C9A2U, 0x0F00F934U, 0x9609A88EU, 0xE10E9818U, 0x7F6A0DBBU, 0x086D3D2DU, 0x91646C97U, 0xE6635C01U,
    0x6B6B51F4U, 0x1C6C6162U, 0x856530D8U, 0xF262004EU, 0x6C0695EDU, 0x1B01A57BU, 0x8208F4C1U, 0xF50FC457U,
    0x65B0D9C6U, 0x12B7E950U, 0x8BBEB8EAU, 0xFCB9887CU, 0x62DD1DDFU, 0x15DA2D49U, 0x8CD37CF3U, 0xFBD44C65U,
    0x4DB26158U, 0x3AB551CEU, 0xA3BC0074U, 0xD4BB30E2U, 0x4ADFA541U, 0x3DD895D7U, 0xA4D1C46DU, 0xD3D6F4FBU,
    0x4369E96AU, 0x346ED9FCU, 0xAD678846U, 0xDA60B8D0U, 0x44042D73U, 0x33031DE5U, 0xAA0A4C5FU, 0xDD0D7CC9U,
    0x5005713CU, 0x270241AAU, 0xBE0B1010U, 0xC90C2086U, 0x5768B525U, 0x206F85B3U, 0xB966D409U, 0xCE61E49FU,
    0x5EDEF90EU, 0x29D9C998U, 0xB0D09822U, 0xC7D7A8B4U, 0x59B33D17U, 0x2EB40D81U, 0xB7BD5C3BU, 0xC0BA6CADU,
    0xEDB88320U, 0x9ABFB3B6U, 0x03B6E20CU, 0x74B1D29AU, 0xEAD54739U, 0x9DD277AFU, 0x04DB2615U, 0x73DC1683U,
    0xE3630B12U, 0x94643B84U, 0x0D6D6A3EU, 0x7A6A5AA8U, 0xE40ECF0BU, 0x9309FF9DU, 0x0A00AE27U, 0x7D079EB1U,
    0xF00F9344U, 0x8708A3D2U, 0x1E01F268U, 0x6906C2FEU, 0xF762575DU, 0x806567CBU, 0x196C3671U, 0x6E6B06E7U,
    0xFED41B76U, 0x89D32BE0U, 0x10DA7A5AU, 0x67DD4ACCU, 0xF9B9DF6FU, 0x8EBEEFF9U, 0x17B7BE43U, 0x60B08ED5U,
    0xD6D6A3E8U, 0xA1D1937EU, 0x38D8C2C4U, 0x4FDFF252U, 0xD1BB67F1U, 0xA6BC5767U, 0x3FB506DDU, 0x48B2364BU,
    0xD80D2BDAU, 0xAF0A1B4CU, 0x36034AF6U, 0x41047A60U, 0xDF60EFC3U, 0xA867DF55U, 0x316E8EEFU, 0x4669BE79U,
    0xCB61B38CU, 0xBC66831AU, 0x256FD2A0U, 0x5268E236U, 0xCC0C7795U, 0xBB0B4703U, 0x220216B9U, 0x5505262FU,
    0xC5BA3BBEU, 0xB2BD0B28U, 0x2BB45A92U, 0x5CB36A04U, 0xC2D7FFA7U, 0xB5D0CF31U, 0x2CD99E8BU, 0x5BDEAE1DU,
    0x9B64C2B0U, 0xEC63F226U, 0x756AA39CU, 0x026D930AU, 0x9C0906A9U, 0xEB0E363FU, 0x72076785U, 0x05005713U,
    0x95BF4A82U, 0xE2B87A14U, 0x7BB12BAEU, 0x0CB61B38U, 0x92D28E9BU, 0xE5D5BE0DU, 0x7CDCEFB7U, 0x0BDBDF21U,
    0x86D3D2D4U, 0xF1D4E242U, 0x68DDB3F8U, 0x1FDA836EU, 0x81BE16CDU, 0xF6B9265BU, 0x6FB077E1U, 0x18B74777U,
    0x88085AE6U, 0xFF0F6A70U, 0x66063BCAU, 0x11010B5CU, 0x8F659EFFU, 0xF862AE69U, 0x616BFFD3U, 0x166CCF45U,
    0xA00AE278U, 0xD70DD2EEU, 0x4E048354U, 0x3903B3C2U, 0xA7672661U, 0xD06016F7U, 0x4969474DU, 0x3E6E77DBU,
    0xAED16A4AU, 0xD9D65ADCU, 0x40DF0B66U, 0x37D83BF0U, 0xA9BCAE53U, 0xDEBB9EC5U, 0x47B2CF7FU, 0x30B5FFE9U,
    0xBDBDF21CU, 0xCABAC28AU, 0x53B39330U, 0x24B4A3A6U, 0xBAD03605U, 0xCDD70693U, 0x54DE5729U, 0x23D967BFU,
    0xB3667A2EU, 0xC4614AB8U, 0x5D681B02U, 0x2A6F2B94U, 0xB40BBE37U, 0xC30C8EA1U, 0x5A05DF1BU, 0x2D02EF8DU
};

unsigned int boot_crc32_update(unsigned int crc, const unsigned char *data, unsigned int len)
{
    for (unsigned int i = 0; i < len; ++i)
    {
        unsigned char index = (unsigned char)((crc ^ data[i]) & 0xFF);
        crc = (crc >> 8) ^ boot_crc32_table[index];
    }
    return crc;
}

void *boot_firmware(void)
{
    int ret                       = 0;
    char *part_name               = HGBOOT_NULL;
    struct firmware_header header = {0};
    struct ota_paramers para      = {0};
    unsigned int retry            = BOOT_MAX_RETRY;
    unsigned int temp_crc32       = 0;
    unsigned int crc32            = 0xffffffff;

    do
    {
        ret = partition_read(PARA_PART, (void *)&para, 0, sizeof(struct ota_paramers));
        if (ret != 0)
        {
            BOOT_WARN("boot read partition %s from offset %d err. %d\r\n", PARA_PART, 0, ret);
            retry--;
            continue;
        }

        BOOT_INFO("boot read partition %s info : para.magic         0x%08x\r\n", PARA_PART, para.magic);
        BOOT_INFO("boot read partition %s info : para.active_slot   0x%08x\r\n", PARA_PART, para.active_slot);
        BOOT_INFO("boot read partition %s info : para.can_be_back   0x%08x\r\n", PARA_PART, para.can_be_back);
        BOOT_INFO("boot read partition %s info : para.upgrade_ready 0x%08x\r\n", PARA_PART, para.upgrade_ready);
        BOOT_INFO("boot read partition %s info : para.app1_crc      0x%08x\r\n", PARA_PART, para.app1_crc);
        BOOT_INFO("boot read partition %s info : para.app2_crc      0x%08x\r\n", PARA_PART, para.app2_crc);
        BOOT_INFO("boot read partition %s info : para.download_crc  0x%08x\r\n", PARA_PART, para.download_crc);

        if (para.magic != OTA_PARA_MAGIC)
        {
            BOOT_WARN("boot read ota parameter magic err. 0x%x != 0x%x\r\n", para.magic, OTA_PARA_MAGIC);
            retry--;
            continue;
        }

        if (para.active_slot == APP_SLOT_1)
        {
            part_name = APP1_PART;
            temp_crc32 = para.app1_crc;
        }
        else
        {
            part_name = APP2_PART;
            temp_crc32 = para.app2_crc;
        }

        BOOT_TRACE("boot will run firmware from partition %s\r\n", part_name);

        ret = partition_read(part_name, (void *)&header, 0, sizeof(struct firmware_header));
        if (ret != 0)
        {
            BOOT_WARN("boot read partition %s from offset %d err. %d\r\n", part_name, 0, ret);
            retry--;
            continue;
        }

        BOOT_INFO("boot read partition %s info : header.magic     0x%08x\r\n", part_name, header.magic);
        BOOT_INFO("boot read partition %s info : header.size      0x%08x\r\n", part_name, header.size);
        BOOT_INFO("boot read partition %s info : header.crc32     0x%08x\r\n", part_name, header.crc32);
        BOOT_INFO("boot read partition %s info : header.version   0x%08x\r\n", part_name, header.version);
        BOOT_INFO("boot read partition %s info : header.load_addr 0x%08x\r\n", part_name, header.load_addr);
        BOOT_INFO("boot read partition %s info : header.exec_addr 0x%08x\r\n", part_name, header.exec_addr);

        if (header.magic != OTA_FIRMWARE_MAGIC)
        {
            BOOT_WARN("boot check firmware magic err. 0x%x != 0x%x\r\n", header.magic, OTA_FIRMWARE_MAGIC);
            BOOT_TRACE("boot try to backup firmware\r\n");
            ret = ota_backup_firmware();
            if (ret != 0)
            {
                BOOT_ERR("boot backup firmware err. %d\r\n", ret);
                return HGBOOT_NULL;
            }
            retry--;
            continue;
        }

        if (header.crc32 != temp_crc32)
        {
            BOOT_WARN("boot check firmware crc32 err. 0x%x != 0x%x\r\n", header.crc32, temp_crc32);
            BOOT_TRACE("boot try to backup firmware\r\n");
            ret = ota_backup_firmware();
            if (ret != 0)
            {
                BOOT_ERR("boot backup firmware err. %d\r\n", ret);
                return HGBOOT_NULL;
            }
            retry--;
            continue;
        }

        ret = partition_read(part_name, (void *)header.load_addr, sizeof(struct firmware_header), header.size);
        if (ret != 0)
        {
            BOOT_WARN("boot read partition %s from offset %d err. %d\r\n", part_name, sizeof(struct firmware_header), ret);
            retry--;
            continue;
        }

        crc32  = 0xffffffff;
        crc32  = boot_crc32_update(crc32, (unsigned char *)header.load_addr, header.size);
        crc32 ^= 0xffffffff;

        if (crc32 != temp_crc32)
        {
            BOOT_WARN("boot check firmware crc32 err. 0x%x != 0x%x\r\n", crc32, temp_crc32);
            BOOT_TRACE("boot try to backup firmware\r\n");
            ret = ota_backup_firmware();
            if (ret != 0)
            {
                BOOT_ERR("boot backup firmware err. %d\r\n", ret);
                return HGBOOT_NULL;
            }
            retry--;
            continue;
        }

        BOOT_INFO("boot finish. firmware exec addr is 0x%x\r\n", header.exec_addr);

        return (void *)header.exec_addr;

    } while(retry > 0);

    BOOT_ERR("boot retry attempts exhausted.");

    return HGBOOT_NULL;
}
