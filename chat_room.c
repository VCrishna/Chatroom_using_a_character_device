#include <linux/init.h>
#include <linux/kfifo.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>


#define MAX_USERS 10
#define FIFO_SIZE 128
struct kfifo_rec_ptr_1 chat_fifo[MAX_USERS];
spinlock_t chat_lock[MAX_USERS];
pid_t chat_ids[MAX_USERS];


static int myopen(struct inode *inode, struct file *file)
{
	int i;
	int ret;
	pid_t pid = get_current()->tgid;
	printk("myopen called in module\n");
	for(i=0;i<MAX_USERS ; i++){
			if(chat_ids[i] == 0){
				chat_ids[i] = pid;
				
				ret = kfifo_alloc(&chat_fifo[i], FIFO_SIZE, GFP_KERNEL);
    		
				if (ret) {
					printk(KERN_ERR "error kfifo_alloc\n");
					return ret;
				}
				spin_lock_init(&chat_lock[i]);

				break;
			}
		}
	return 0;
}

static int myclose(struct inode *inodep, struct file *filp)
{
	int i;
	pid_t pid = get_current()->tgid;
	printk("myclose called in module\n");
	
	i=0;
	while(i < MAX_USERS){
		if(chat_ids[i] == pid){
			kfifo_free(&chat_fifo[i]);
			chat_ids[i] = 0;
			break;
		}
		i++;
	}
	return 0;
}


static ssize_t mywrite(struct file *file, const char __user *buf, size_t len, loff_t *ppos)
{
	char *module_buf;
	char temp_buf[100];
	int message_length;
	pid_t pid = get_current()->tgid;
	int user_name = -1;
	int i;

	for(i=0;i<MAX_USERS ; i++){
		if(chat_ids[i] == pid){
			user_name = i;//user index
			break;
		}
	}


	printk("mywrite function called in module\n");

	module_buf = (char *)kmalloc(len, GFP_USER);
	memset(module_buf,'\0', sizeof(char)*len);
	if(!module_buf) 
		return -1; //kmalloc failed

	len = copy_from_user(module_buf, buf, len);//copying to module_buf
	

	
	
	temp_buf[0] = 'P';
	temp_buf[1] = '0' + user_name;
	temp_buf[2] = ' ';
	temp_buf[3] = ':';
	temp_buf[4] = ' ';
	temp_buf[5] = '\0';

	strcat(temp_buf,module_buf);
	message_length = strlen(temp_buf) + 1;
	temp_buf[message_length] = '\0';
	for(i = 0; i<MAX_USERS;i++){
		if(chat_ids[i] != 0 && chat_ids[i] != pid)
			kfifo_in_spinlocked(&chat_fifo[i], temp_buf, message_length, &chat_lock[i]);
	}
	
	kfree(module_buf); // clean up

	return len; 

}

static ssize_t myread(struct file *file, char __user *buf, size_t len, loff_t *ppos)
{

	char *module_buf;
	int buflen = 100;
	int message_length;
	int ret;
	pid_t pid = get_current()->tgid;
	int user_name = -1;
	int i;
	

	//printk("myread function called in module\n");
	
	for(i=0;i<MAX_USERS ; i++){
		if(chat_ids[i] == pid){
			user_name = i;
			break;
		}
	}

	if( user_name == -1 ){
		return -1;
	}
	
	if( len < buflen )
		return -1; // not enough space in user buffer to return string.

	module_buf = (char *)kmalloc(buflen, GFP_USER);
	memset(module_buf,'\0', sizeof(char)*buflen);
	if( !module_buf ) 
		return -1; //kmalloc failed

	//printk("myread function called in module %d \n", user_name);
	sprintf(module_buf,"none");
	message_length=5;

	if(!kfifo_is_empty(&chat_fifo[user_name])) {
        ret = kfifo_out_spinlocked(&chat_fifo[user_name], module_buf, buflen, &chat_lock[user_name]);
        module_buf[ret] = '\0';
        message_length = strlen(module_buf);		
	}
	
	len = copy_to_user(buf, module_buf, message_length);

	kfree(module_buf); // clean up

	return len; 
}

static const struct file_operations myfops = {
    .owner	= THIS_MODULE,
    .read	= myread,
    .write	= mywrite,
    .open	= myopen,
    .release	= myclose,
    .llseek 	= no_llseek,
};

struct miscdevice mydevice = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "chat_room",
    .fops = &myfops,
    .mode = S_IRUGO | S_IWUGO,
};

static int __init my_init(void)
{
	int i =0;

	i=0;
	while(i < MAX_USERS){
		chat_ids[i] = 0;
		i++;
	}

	printk("my_init called\n");

	// register the character device
	if (misc_register(&mydevice) != 0) {
		printk("device registration failed\n");
		return -1;
	}

	printk("character device registered\n");

	return 0;
}

static void __exit my_exit(void)
{
	printk("my_exit called\n");

	
	
	misc_deregister(&mydevice);
}

module_init(my_init)
module_exit(my_exit)
MODULE_DESCRIPTION("Miscellaneous character device module\n");
MODULE_AUTHOR("Sai Krishna");
MODULE_LICENSE("GPL");

