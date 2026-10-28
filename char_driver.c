#include <linux/init.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

//stop compiler from complaining
MODULE_AUTHOR("Your Name");
MODULE_LICENSE("GPL");

#define DEVICE_NAME "simple_char_driver"
#define MAJOR_NUM = 511
#define BUFFER_SIZE 900

static char *mainBuffer = NULL;
static int openCount = 0;
static int closeCount = 0;

ssize_t pa2_char_driver_read (struct file *pfile, char __user *buffer, size_t length, loff_t *offset)
{
	/* *buffer is the userspace buffer to where you are writing the data you want to be read from the device file*/
	/* length is the length of the userspace buffer*/
	/* offset will be set to current position of the opened file after read*/
	/* copy_to_user function: source is device_buffer and destination is the userspace buffer *buffer */

	int amountOfBytesToRead;
	int bytesTransfer;
	int readBytes;

	if(*offset >= BUFFER_SIZE || *offset < 0) //too big compared to buffer, or offset is negative
	{
		printk(KERN_ALERT "simple_char_driver: 0 bytes read from device\n");
		return 0;
	}

	amountOfBytesToRead = BUFFER_SIZE - *offset;
	if(amountOfBytesToRead >= length) //cap to length
	{
		amountOfBytesToRead = length;
	}
	else //overflow
	{
		printk(KERN_ALERT "simple_char_driver: reading %ld bytes will overflow the buffer\n", length);
	}

	bytesTransfer = copy_to_user(buffer, mainBuffer + *offset, amountOfBytesToRead);
	if(bytesTransfer != 0) //fail
	{
		printk(KERN_ALERT "simple_char_driver: failed to copy %d bytes to user\n", amountOfBytesToRead);
		return -EFAULT;
	}

	readBytes = amountOfBytesToRead - bytesTransfer;
	*offset = *offset + readBytes;

	printk(KERN_ALERT "simple_char_driver: %d bytes read from device\n", readBytes);

	return readBytes;
}

ssize_t pa2_char_driver_write (struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{
	/* *buffer is the userspace buffer where you are writing the data you want to be written in the device file*/
	/* length is the length of the userspace buffer*/
	/* current position of the opened file*/
	/* copy_from_user function: destination is device_buffer and source is the userspace buffer *buffer */

	int amountOfBytesToWrite;
	int bytesTransfer;
	int writtenBytes;

	if(*offset >= BUFFER_SIZE) //too big compared to buffer
	{
		printk(KERN_ALERT "simple_char_driver: buffer is full\n");
		return -ENOSPC;
	}

	if(*offset < 0)
	{
		printk(KERN_ALERT "simple_char_driver: invalid offset\n");
		return -EINVAL;
	}

	amountOfBytesToWrite = BUFFER_SIZE - *offset;
	if(amountOfBytesToWrite >= length) //cap to length
	{
		amountOfBytesToWrite = length;
	}
	else
	{
		printk(KERN_ALERT "simple_char_driver: writing %ld bytes will overflow the buffer\n", length);
	}

	bytesTransfer = copy_from_user(mainBuffer + *offset, buffer, amountOfBytesToWrite);
	if(bytesTransfer != 0) //fail
	{
		printk(KERN_ALERT "simple_char_driver: failed to copy %d bytes from user\n", amountOfBytesToWrite);
		return -EFAULT;
	}

	writtenBytes = amountOfBytesToWrite - bytesTransfer;
	*offset = *offset + writtenBytes;

	printk(KERN_ALERT "simple_char_driver: %d bytes written to device\n", writtenBytes);

	return writtenBytes;
}

int pa2_char_driver_open (struct inode *pinode, struct file *pfile)
{
	openCount = openCount + 1;
	printk(KERN_ALERT "simple_char_driver: Device has now been opened %d times\n", openCount);
	return 0;
}

int pa2_char_driver_close (struct inode *pinode, struct file *pfile)
{
	closeCount = closeCount + 1;
	printk(KERN_ALERT "simple_char_driver: Device has now been closed %d times\n", closeCount);
	return 0;
}

loff_t pa2_char_driver_seek (struct file *pfile, loff_t offset, int whence)
{
	/* Update open file position according to the values of offset and whence */
	loff_t newPosition = 0;

	switch(whence)
	{
		case 0: //simply set position to be offset
			newPosition = offset;
			break;
		case 1: //add/subtract offset from a position depending on if offset is positive or negative
			newPosition = pfile->f_pos + offset;
			break;
		case 2: //set position to be added/subtracted by offset at the end of the file
			newPosition = BUFFER_SIZE + offset;
			break;
		default: //invalid whence passed
			return -EINVAL;
	}

	if(newPosition >= BUFFER_SIZE)
	{
		printk(KERN_ALERT "simple_char_driver: position %lld not in range\n", newPosition);
		newPosition = BUFFER_SIZE;
	}
	else if(newPosition < 0)
	{
		printk(KERN_ALERT "simple_char_driver: position %lld not in range\n", newPosition);
		newPosition = 0;
	}


	pfile->f_pos = newPosition;
	printk(KERN_ALERT "simple_char_driver: device now at position %lld\n", newPosition);
	return newPosition;
}

struct file_operations pa3_char_driver_file_operations = {
	.owner   = THIS_MODULE,
	.open = pa2_char_driver_open,
    .release = pa2_char_driver_close,
    .read = pa2_char_driver_read,
    .write = pa2_char_driver_write,
    .llseek = pa2_char_driver_seek
};

static int pa2_char_driver_init(void)
{
	int registerResult;

	mainBuffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
	if(!mainBuffer) //malloc failed
	{
		printk(KERN_ALERT "simple_char_driver: failed to allocate memory\n");
		return -ENOMEM;
	}

	registerResult = register_chrdev(511, "simple_char_driver", &pa3_char_driver_file_operations);
	if(registerResult < 0) //just in case it fails somehow
	{
		printk(KERN_ALERT "simple_char_driver: failed: %d\n", registerResult);
		return registerResult;
	}
	else
	{
		printk(KERN_ALERT "simple_char_driver: initialized: %d\n", registerResult);
		return 0;
	}
}

static void pa2_char_driver_exit(void)
{
	unregister_chrdev(511, "simple_char_driver");
	if(mainBuffer)
	{
		kfree(mainBuffer);
	}

	printk(KERN_ALERT "simple_char_driver: exited\n");
}

module_init(pa2_char_driver_init);
module_exit(pa2_char_driver_exit);