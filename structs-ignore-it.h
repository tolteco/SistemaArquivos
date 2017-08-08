


typedef struct NFS_info_t
{
	unsigned char	block_size; 	
	unsigned char	n_blocks_per_group;
	unsigned short	n_groups;				
}__attributes__((packed)) info_t;


enum class entry_flags : unsigned char
{
	READ_ONLY	= 0x01,
	HIDDEN		= 0x02,
	DIRECTORY	= 0x04,
	FILE		= 0x08,
	READ		= 0x10,
	WRITE		= 0x20,
	EXECUTABLE	= 0x40/*,
	XXXXXXXXXXXX= 0X80*/
}


typedef struct NFS_date_time
{
	unsigned year			: 12;	
	unsigned month			: 4;	
	unsigned day			: 5;	
	unsigned hour			: 5;	
	unsigned minute			: 6;	
	unsigned second			: 6;	
	unsigned millisecond	: 10;
}__attribute__((packed)) date_time;


typedef struct NFS_entry_t
{
	char					name_length;
	char					name[32];				
	sfs_entry_attributes	attributes;				
	sfs_date_time			creation_time;		
	sfs_date_time			modification_time;	
	sfs_date_time			last_access_time;	
	unsigned long long		size;					
	unsigned int			first_block;			
}__attributes__((packed)) entry_t;


bool format(FILE* pointer, const info_t& info);