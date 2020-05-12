#include <linux/cdev.h>
#include <linux/fcntl.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <asm/atomic.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/cdev.h>	
#include <linux/sched.h>	
#include <asm/pgtable.h>

#include "ioctl_dev.h"
#include "ioctl.h"

#define BUF_LEN 80

static char Message[BUF_LEN];

static char *Message_Ptr;

/* store the major number extracted by dev_t */
int ioctl_d_interface_major = 0;
int ioctl_d_interface_minor = 0;

#define DEVICE_NAME "ioctl_d"
char* ioctl_d_interface_name = DEVICE_NAME;

static char *Message_Ptr;


ioctl_d_interface_dev ioctl_d_interface;

struct file_operations ioctl_d_interface_fops = {
	.owner = THIS_MODULE,
	.read = NULL,
	.write = NULL,
	.open = ioctl_d_interface_open,
	.unlocked_ioctl = ioctl_d_interface_ioctl,
	.release = ioctl_d_interface_release
};

/* Private API */
static int ioctl_d_interface_dev_init(ioctl_d_interface_dev* ioctl_d_interface);
static void ioctl_d_interface_dev_del(ioctl_d_interface_dev* ioctl_d_interface);
static int ioctl_d_interface_setup_cdev(ioctl_d_interface_dev* ioctl_d_interface);
static int ioctl_d_interface_init(void);
static void ioctl_d_interface_exit(void);



static struct page *walk_page_table(unsigned long addr)
{
    pgd_t *pgd;
    pte_t *ptep, pte;
    pud_t *pud;
    pmd_t *pmd;
    dma_addr_t phys_addr;

    struct page *page = NULL;
    struct mm_struct *mm = current->mm;

    pgd = pgd_offset(mm, addr);
    if (pgd_none(*pgd) || pgd_bad(*pgd))
        goto out;
    printk(KERN_NOTICE "Valid pgd");

    pud = pud_offset(pgd, addr);
    if (pud_none(*pud) || pud_bad(*pud))
        goto out;
    printk(KERN_NOTICE "Valid pud");

    pmd = pmd_offset(pud, addr);
    if (pmd_none(*pmd) || pmd_bad(*pmd))
        goto out;
    printk(KERN_NOTICE "Valid pmd");

    ptep = pte_offset_map(pmd, addr);
    if (!ptep)
        goto out;
    pte = *ptep;

    page = pte_page(pte);
    if (page)
        printk(KERN_INFO "page frame struct is @ %p", page);
        phys_addr = page_to_phys(page);
        printk(KERN_INFO "physical addr is @ %p", phys_addr);

 out:
    return page;
}



static int ioctl_d_interface_dev_init(ioctl_d_interface_dev * ioctl_d_interface)
{
	int result = 0;
	memset(ioctl_d_interface, 0, sizeof(ioctl_d_interface_dev));
	atomic_set(&ioctl_d_interface->available, 1);
	sema_init(&ioctl_d_interface->sem, 1);

	return result;
}

static void ioctl_d_interface_dev_del(ioctl_d_interface_dev * ioctl_d_interface) {}

static int ioctl_d_interface_setup_cdev(ioctl_d_interface_dev * ioctl_d_interface)
{
  int error = 0;
	dev_t devno = MKDEV(ioctl_d_interface_major, ioctl_d_interface_minor);

	cdev_init(&ioctl_d_interface->cdev, &ioctl_d_interface_fops);
	ioctl_d_interface->cdev.owner = THIS_MODULE;
	ioctl_d_interface->cdev.ops = &ioctl_d_interface_fops;
	error = cdev_add(&ioctl_d_interface->cdev, devno, 1);

	return error;
}

static int ioctl_d_interface_init(void)
{
	dev_t           devno = 0;
	int             result = 0;

	ioctl_d_interface_dev_init(&ioctl_d_interface);


	/* register char device
	 * we will get the major number dynamically this is recommended see
	 * book : ldd3
	 */
	result = alloc_chrdev_region(&devno, ioctl_d_interface_minor, 1, ioctl_d_interface_name);
	ioctl_d_interface_major = MAJOR(devno);
	if (result < 0) {
		printk(KERN_WARNING "ioctl_d_interface: can't get major number %d\n", ioctl_d_interface_major);
		goto fail;
	}

	result = ioctl_d_interface_setup_cdev(&ioctl_d_interface);
	if (result < 0) {
		printk(KERN_WARNING "ioctl_d_interface: error %d adding ioctl_d_interface", result);
		goto fail;
	}

	printk(KERN_INFO "ioctl_d_interface: module loaded\n");
	return 0;

fail:
	ioctl_d_interface_exit();
	return result;
}

static void ioctl_d_interface_exit(void)
{
	dev_t devno = MKDEV(ioctl_d_interface_major, ioctl_d_interface_minor);

	cdev_del(&ioctl_d_interface.cdev);
	unregister_chrdev_region(devno, 1);
	ioctl_d_interface_dev_del(&ioctl_d_interface);

	printk(KERN_INFO "ioctl_d_interface: module unloaded\n");
}

/* Public API */
int ioctl_d_interface_open(struct inode *inode, struct file *filp)
{
	ioctl_d_interface_dev *ioctl_d_interface;

	ioctl_d_interface = container_of(inode->i_cdev, ioctl_d_interface_dev, cdev);
	filp->private_data = ioctl_d_interface;

	if (!atomic_dec_and_test(&ioctl_d_interface->available)) {
		atomic_inc(&ioctl_d_interface->available);
		printk(KERN_ALERT "open ioctl_d_interface : the device has been opened by some other device, unable to open lock\n");
		return -EBUSY;		/* already open */
	}

        Message_Ptr = Message;	

	return 0;
}

int ioctl_d_interface_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
int bytes_read = 0;


  printk("ioctl_d_interface_read called \n");
if(*Message_Ptr == 0)
  return 0;

while(length && *Message_Ptr){
   put_user(*(Message_Ptr++), buffer++);
   length--;
   bytes_read++;
   }

   printk("Read %d bytes, %d left \n",bytes_read, length);
   return bytes_read;
}



int ioctl_d_interface_release(struct inode *inode, struct file *filp)
{
	ioctl_d_interface_dev *ioctl_d_interface = filp->private_data;
	atomic_inc(&ioctl_d_interface->available);	/* release the device */
	return 0;
}

long ioctl_d_interface_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
		// Get the number of channel found
		case IOCTL_BASE_GET_MUIR:
			printk(KERN_INFO "<%s> ioctl: IOCTL_BASE_GET_MUIR\n", DEVICE_NAME);
			uint32_t value = 0xdeadbeaf;
			if (copy_to_user((uint32_t*) arg, &value, sizeof(value))){
				return -EFAULT;
			}
			break;

		case IOCTL_BASE_PAGE_TABLE:
                       printk(KERN_INFO "<> ioctl: IOCTL_PAGE_TABLE\n");
		       struct page *page = walk_page_table(arg);
		default:
			break;
	}

	return 0;
}


module_init(ioctl_d_interface_init);
module_exit(ioctl_d_interface_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Florian Depraz <florian.depraz@outlook.com>");
MODULE_DESCRIPTION("IOCTL Interface driver");
MODULE_VERSION("0.1");

