/* Record v1.0 by Karl Stenerud.
 *
 * Allows basic management of an S-Record file.
 * Operations supported:
 *		-add a file as a new segment in the S-Record file.
 *		-extract a segment and save it to a binary file.
 *		-delete a segment from the S-Record file.
 *		-list the contents of an S-Record file to stdout.
 *		-print the contents of a segment to stdout.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* S-Record Format:
 *
 * S T BC ADDR DATA CS (CR LF)
 *
 * S    (1)          = 'S'
 * T    (1)          = Record Type ('1', '2', '3', '7', '8', '9')
 * BC   (2)          = Byte Count (length of ADDR + DATA + CS)
 * ADDR (4, 6, or 8) = Address to write data
 * DATA (ADDR)       = Data to write
 * CS   (2)          = Checksum (one's complement of sum of BC, ADDR, and DATA)
 * CR   (1)          = Carriage Return (0x0d) (optional)
 * LF   (1)          = Linefeed (0x0a) (optional)
 */

#define SREC_SUCCESS					0
#define SREC_ERR_NULL					-1	/* Function was called with a NULL pointer */
#define SREC_ERR_INVALID_RECORD_TYPE	-2	/* Invalid record type for this operation */
#define SREC_ERR_NOT_S_RECORD			-3	/* Line did not start with 'S' */
#define SREC_ERR_TOO_LONG				-4	/* Langth of data segment is too long to fit */
#define SREC_ERR_FAILED_CHECKSUM		-5	/* Checksum field and calculated checksum don't match */
#define SREC_ERR_FILE_OPEN_READ			-6	/* Could not open file for reading */
#define SREC_ERR_FILE_OPEN_WRITE		-7	/* Could not open file for writing */
#define SREC_ERR_FILE_READ				-8	/* Could not read from file */
#define SREC_ERR_FILE_WRITE				-9	/* Could not write to file */
#define SREC_ERR_NO_MEMORY				-10	/* Unable to allocate memory */
#define SREC_ERR_LIST_EMPTY				-11	/* Tried to access an empty list */
#define SREC_ERR_BAD_COUNT				-12	/* Record count in s5 record is different fron ours */
#define SREC_ERR_EXTRA_TERMINATOR		-13	/* Got a second terminator but no data */
#define SREC_ERR_INVALID_MODULE_NUMBER	-14	/* Asked for a module that doesn't exist */
#define SREC_ERR_INVALID_SEGMENT_NUMBER	-15	/* Asked for a segment that doesn't exist */

/* The maximum allowed raw data length when encoding an S-Record */
#define SREC_MAX_DATA_LENGTH 64

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

/* Check if an ascii value is numeric (0-9) */
#define IS_DIG_NUM(A)    ( 	(A) >= '0' && (A) <= '9' )
/* Check if an ascii value is hex > 9 (A-F) */
#define IS_DIG_HEX(A)    (	(A) >= 'A' && (A) <= 'F' )
/* Check if an ascii value is a hex value */
#define IS_VALID_HEX(A)  (	IS_DIG_NUM(A) || IS_DIG_HEX(A) )
/* Decode 1 ascii char into a nybble */
#define GET_HEX(A)       (	IS_DIG_NUM(A) ? (A)-'0' : (A)-'A'+10 )
/* Decode 2 ascii chars into a byte */
#define GET_HEX_8BIT(A)  (	((GET_HEX(*(A))<<4)      |  GET_HEX(*((A)+1)))&0xff )
/* Decode 4 ascii chars into 2 bytes */
#define GET_HEX_16BIT(A) (	((GET_HEX(*(A))<<12)     | (GET_HEX(*((A)+1))<<8)  | \
							 (GET_HEX(*((A)+2))<<4)  |  GET_HEX(*((A)+3)))&0xffff )
/* Decode 6 ascii chars into 3 bytes */
#define GET_HEX_24BIT(A) (	((GET_HEX(*(A))<<20)     | (GET_HEX(*((A)+1))<<16) | \
							 (GET_HEX(*((A)+2))<<12) | (GET_HEX(*((A)+3))<<8)  | \
							 (GET_HEX(*((A)+4))<<4)  |  GET_HEX(*((A)+5)))&0xffffff )
/* Decode 8 ascii chars into 4 bytes */
#define GET_HEX_32BIT(A) (	((GET_HEX(*(A))<<28)     | (GET_HEX(*((A)+1))<<24) | \
							 (GET_HEX(*((A)+2))<<20) | (GET_HEX(*((A)+3))<<16) | \
							 (GET_HEX(*((A)+4))<<12) | (GET_HEX(*((A)+5))<<8)  | \
							 (GET_HEX(*((A)+6))<<4)  |  GET_HEX(*((A)+7)))&0xffffffff )

/* Get the record type from an S-Record */
#define SREC_GET_TYPE(A)       GET_HEX(*((A)+1))
/* Get the byte count from an S-Record */
#define SREC_GET_BYTE_COUNT(A) GET_HEX_8BIT((A)+2)
/* Get the address (16-bit) from an S-Record */
#define SREC_GET_ADDRESS_16(A) GET_HEX_16BIT((A)+4)
/* Get the address (24-bit) from an S-Record */
#define SREC_GET_ADDRESS_24(A) GET_HEX_24BIT((A)+4)
/* Get the address (32-bit) from an S-Record */
#define SREC_GET_ADDRESS_32(A) GET_HEX_32BIT((A)+4)
/* Get the checksum from an S-Record */
#define SREC_GET_CHECKSUM(A)   GET_HEX_8BIT((A)+2+(SREC_GET_BYTE_COUNT(A)<<1))


/* Holds one S-Record and provides the abilities for a linked list. */
typedef struct Srec_Record
{
	struct Srec_Record* next;	/* Next in linked list */
	struct Srec_Record* prev;	/* Previous in linked list */
	unsigned int type;			/* record type */
	unsigned int length;		/* Length of data */
	unsigned int address;		/* Address for this record */
	char data[256];				/* The data (256 just in case) */
} srec_record;

/* Points to a segment of data */
typedef struct Srec_Segment
{
	struct Srec_Segment* next;	/* Next in linked list */
	struct Srec_Segment* prev;	/* Previous in linked list */
	srec_record* start;			/* Starting record of this segment */
	srec_record* end;			/* Ending record of this segment */
	unsigned int execute_address;	/* Address where cde execution begins */
	unsigned int type;			/* Record type used for this segment */
} srec_segment;

/* Holds one module and provides the abilities for a linked list. */
typedef struct Srec_Module
{
	struct Srec_Module* next;	/* Next in linked list */
	struct Srec_Module* prev;	/* Previous in linked list */
	srec_record header;			/* Holds one s0 record */
	srec_record record_list;	/* Holds a list of data records */
								/* This is the head of the list and holds no data. */
	srec_segment segment_list;	/* List of data segments */
} srec_module;


int add_binary_file(char* filename, char* module_name, unsigned int address, unsigned int record_data_length, unsigned int record_type, unsigned int execute_address, unsigned int module_num);
int write_binary_file(int module_num, int segment_num, char* filename);

int          read_srecord_file(char* filename);
int          write_srecord_file(char* filename);

int          write_s_record(FILE* file, srec_record* record);

int          encode_s_record(char* srecord, srec_record* record);
int          decode_s_record(char* srecord, srec_record* record);

srec_module* get_module_by_number(unsigned int number);
srec_segment* get_segment_by_number(srec_module* module, unsigned int number);

srec_module* new_module();
int          add_module(srec_module* module, srec_module* list_location);
int          remove_module(srec_module* module);
int          delete_module(srec_module* module);

srec_segment* new_segment();
int          add_segment(srec_segment* segment, srec_segment* list_location);
int          remove_segment(srec_segment* segment);
int          delete_segment(srec_segment* segment);

srec_record* new_record();
int          add_record(srec_record* record, srec_record* list_location);
int          remove_record(srec_record* record);
int          delete_record(srec_record* record);

unsigned int get_whitespace_length(char* buff, unsigned int length);
unsigned int calc_checksum(char* s_record);
void         ascii_to_hex(char* ascii, char* hex, unsigned int ascii_length);
void         hex_to_ascii(char* hex, char* ascii, unsigned int hex_length);

int make_segments(srec_module* module);


/* The module list */
srec_module g_module_list = {&g_module_list, &g_module_list};



/* Read a binary file into a module structure.
 *
 * Data:	filename:			The file to write to.
 *			module_name:		The module name to use if this is a new module.
 *			address:			The address where this data will be loaded.
 *			record_data_length:	Data length to use when making S-Records.
 *			record_type:		Record type to use for data records (1-3).
 *			execute_address:	Address where code execution begins.
 *			module_num:			Module to load data into (0 = make new module).
 *
 * Return	int:	The number of bytes read or an error message (negative value)
 *
 * Notes:	If module_num is 0, a new module will be created with module_name as its name.
 *			record_data_length specifies the maximum size to use when creating each
 *			S-Record.
 */
int add_binary_file(char* filename, char* module_name, unsigned int address, unsigned int record_data_length, unsigned int record_type, unsigned int execute_address, unsigned int module_num)
{
	FILE* data_file;				/* File to read from */
	char buff[200];					/* Temporary buffer */
	srec_record* record;			/* Record we are working with */
	srec_module* module;			/* Module we are working with */
	unsigned int bytes_read;		/* Bytes read in last file read */
	unsigned int total_read = 0;	/* Total bytes read */
	int rc;							/* Return code from function calls */
	srec_segment* segment;			/* Current data segment */
	unsigned int first_record = 1;	/* Flag if this is the first record to add */

	/* Sanity checks */
	if(filename == NULL)
		return SREC_ERR_NULL;

	if(record_data_length > SREC_MAX_DATA_LENGTH)
		return SREC_ERR_TOO_LONG;

	if(record_type < 1 && record_type > 3)
		return SREC_ERR_INVALID_RECORD_TYPE;

	/* Open the file */
	if((data_file = fopen(filename, "rb")) == NULL)
		return SREC_ERR_FILE_OPEN_READ;

	if(module_num > 0)
	{
		/* We want to load this data into a specific module */
		if((module = get_module_by_number(module_num)) == NULL)
		{
			fclose(data_file);
			return SREC_ERR_INVALID_MODULE_NUMBER;
		}
	}
	else
	{
		/* Make a new module to store that data in */
		if((module = new_module()) == NULL)
		{
			fclose(data_file);
			return SREC_ERR_NO_MEMORY;
		}
		if((rc = add_module(module, g_module_list.prev)) < 0)
		{
			fclose(data_file);
			return rc;
		}
		strcpy(module->header.data, module_name);
		module->header.length = strlen(module_name);
	}

	/* Make a new segment for the data */
	if((segment = new_segment()) == NULL)
	{
		fclose(data_file);
		return SREC_ERR_NO_MEMORY;
	}
	if((rc = add_segment(segment, module->segment_list.prev)) < 0)
	{
		fclose(data_file);
		return rc;
	}
	segment->execute_address = execute_address;

	/* Read the file in chunks for data records */
	while((bytes_read = fread(buff, 1, record_data_length, data_file)) > 0)
	{
		/* Make a new record */
		if((record = new_record()) == NULL)
		{
			fclose(data_file);
			return SREC_ERR_NO_MEMORY;
		}
		if((rc = add_record(record, module->record_list.prev)) < 0)
		{
			fclose(data_file);
			return rc;
		}
		/* Store the info needed to make an s-record */
		record->type = record_type;
		record->length = bytes_read;
		record->address = address;
		/* Now copy the actual data */
		memcpy(record->data, buff, bytes_read);
		/* Advance the address */
		address += bytes_read;
		total_read += bytes_read;
		if(first_record)
		{
			segment->start = record;
			first_record = 0;
		}
	}
	/* Mark the end of the segment */
	segment->end = record;
	return total_read;
}


/* Write a module to a binary file.
 *
 * Data:	module_num:		Module to save data from.
 *			segment_num:	Segment to save data from.
 *			filename:		name of file to save to.
 *
 * Return	int:	The number of bytes written or an error message (negative value)
 *
 */
int write_binary_file(int module_num, int segment_num, char* filename)
{
	FILE* data_file;				/* The file to write to */
	srec_record* record;			/* The record we are working with */
	unsigned int bytes_written;		/* Bytes written in last write to file */
	unsigned int total_written = 0;	/* Total bytes written to file */
	srec_module* module;			/* Current module */
	srec_segment* segment;			/* Current segment */

	/* Get the requested module and segment */
	if((module = get_module_by_number(module_num)) == NULL)
		return SREC_ERR_INVALID_MODULE_NUMBER;
	
	if((segment = get_segment_by_number(module, segment_num)) == NULL)
		return SREC_ERR_INVALID_SEGMENT_NUMBER;

	/* Open the file */
	if(filename != NULL)
	{
		if((data_file = fopen(filename, "wb")) == NULL)
			return SREC_ERR_FILE_OPEN_WRITE;
	}
	else
		data_file = stdout;

	/* Look at each record in the segment */
	for(record = segment->start;record != segment->end->next;record = record->next)
	{
		/* Write the contents to file */
		if((bytes_written = fwrite(record->data, 1, record->length, data_file)) != record->length)
		{
			fclose(data_file);
			return SREC_ERR_FILE_WRITE;
		}
		/* Keep totals */
		total_written += bytes_written;
	}
	return total_written;
}


/* Read an s-record file.
 *
 * Data:	filename:		name of file to read from.
 *
 * Return	int:	The number of bytes read or an error message (negative value)
 *
 */
int read_srecord_file(char* filename)
{
	FILE* srecord_file;				/* The s-record file */
	char buff[1000];				/* Temporary buffer */
	char* ptr = buff;				/* pointers to allow buffered reads */
	char* load_pos = buff;
	char* buff_end = buff + 1000;
	int read_length;				/* Bytes read by last read */
	int decode_length;				/* Length of the decoded data */
	srec_module* module;			/* Module we are working with */
	srec_record* record;			/* Record we are working with */
	srec_record temp_record;		/* Temporary record */
	unsigned int record_count = 0;	/* Counts number of data records read */
	unsigned int last_count_check = 0;	/* Updated every time S5 is encountered */
	srec_segment* segment = NULL;	/* Current segment */
	int rc;							/* Return code from function calls */

	/* Sanity check */
	if(filename == NULL)
		return SREC_ERR_NULL;

	/* Open the s-record file */
	if((srecord_file = fopen(filename, "rt")) == NULL)
		return SREC_ERR_FILE_OPEN_READ;

	/* Initial read */
	if((read_length = fread(load_pos, 1, 1000, srecord_file)) <= 0)
	{
		fclose(srecord_file);
		return SREC_ERR_FILE_READ;
	}
	/* Update pointers */
	buff_end = load_pos + read_length;
	ptr += get_whitespace_length(ptr, buff_end - ptr);

	/* If there's no S0 record, make a default module */
	if(ptr[1] != '0')
	{
		if((module = new_module()) == NULL)
		{
			fclose(srecord_file);
			return SREC_ERR_NO_MEMORY;
		}
		if((rc = add_module(module, g_module_list.prev)) < 0)
		{
			delete_module(module);
			fclose(srecord_file);
			return rc;
		}
	}

	/* Read through this buffer */
	while(ptr < buff_end)
	{
		/* Low water mark so we don't accidentally run off the end of the buffer */
		/* If read_length is 0, there's no more data in the file */
		if((buff_end - ptr) < 300 && read_length > 0)
		{
			/* Move the unread data to the bottom of the buffer */
			memcpy(buff, ptr, buff_end - ptr);
			load_pos = buff + (buff_end - ptr);
			/* Read more data into the rest of the buffer */		
			read_length = fread(load_pos, 1, ptr - buff, srecord_file);
			/* Update the pointers */
			buff_end = load_pos + read_length;
			ptr = buff;
		}

		/* Decode an s-record into the temporary recrd */
		if((decode_length = decode_s_record(ptr, &temp_record)) < 0)
		{
			fclose(srecord_file);
			return decode_length;
		}
		/* Advance our buffer pointer  */
		ptr += decode_length;
		/* Bypass any whitespace */
		ptr += get_whitespace_length(ptr, buff_end - ptr);

		/* Process based on record type */
		switch(temp_record.type)
		{
			case 0:	/* S0: Module header */
				/* Make a new module */
				if((module = new_module()) == NULL)
				{
					fclose(srecord_file);
					return SREC_ERR_NO_MEMORY;
				}
				if((rc = add_module(module, g_module_list.prev)) < 0)
				{
					delete_module(module);
					fclose(srecord_file);
					return rc;
				}
				/* Null terminate to make it easier to work with */
				temp_record.data[temp_record.length] = 0;
				/* Store module name */
				module->header = temp_record;
				break;
			case 1:	/* S1: Data record with 16-bit address */
			case 2:	/* S2: Data record with 24-bit address */
			case 3:	/* S3: Data record with 32-bit address */
				/* Make a record to store the information */
				if((record = new_record()) == NULL)
				{
					fclose(srecord_file);
					return SREC_ERR_NO_MEMORY;
				}

				*record = temp_record;

				if((rc = add_record(record, module->record_list.prev)) < 0)
				{
					delete_record(record);
					fclose(srecord_file);
					return rc;
				}

				/* If there's no current segment, make one */
				if(segment == NULL)
				{
					if((segment = new_segment()) == NULL)
					{
						fclose(srecord_file);
						return SREC_ERR_NO_MEMORY;
					}

					segment->start = record;

					if((rc = add_segment(segment, module->segment_list.prev)) < 0)
					{
						delete_segment(segment);
						fclose(srecord_file);
						return rc;
					}
				}

				/* Increase the record count */
				record_count++;
				break;
			case 5:	/* S5: count record */
				/* Compare the count in the S5 record to the actual number
				 * of recrods we have read.  If they are not equal, the file
				 * has been truncated or corrupted in some way.
				 */
				if((record_count - last_count_check) != temp_record.address)
				{
					fclose(srecord_file);
					return SREC_ERR_BAD_COUNT;
				}
				last_count_check = record_count;
				break;
			case 7:	/* S7: Terminator record for S3 records */
			case 8:	/* S8: Terminator record for S2 records */
			case 9:	/* S9: Terminator record for S1 records */
				/* End this segment */
				if(segment == NULL)
				{
					fclose(srecord_file);
					return SREC_ERR_EXTRA_TERMINATOR;
				}
				segment->end = record;
				segment->execute_address = temp_record.address;
				segment = NULL;
				break;
			default:
				fclose(srecord_file);
				return SREC_ERR_INVALID_RECORD_TYPE;
		}
	}
	fclose(srecord_file);

	/* Handle badly behaved files */
	if(segment != NULL)
		segment->end = record;

	return record_count;
}


/* Write everything to an S-Record file.
 *
 * Data:	filename:		name of file to write to.
 *
 * Return	int:	The number of bytes written or an error message (negative value)
 */
int write_srecord_file(char* filename)
{
	FILE* srecord_file;				/* The S-Record file */
	srec_module* module;			/* The module we are working with */
	srec_record* record;			/* The record we are working with */
	unsigned int bytes_written = 0;	/* Total of the bytes written to file */
	int rc;							/* Return code from write_s_record() */
	unsigned int num_records = 0;	/* Number of data records written */
	srec_record temp_record = {0};	/* Temporary record */
	srec_segment* segment;			/* Current data segment */

	/* Sanity check */
	if(filename == NULL)
		return SREC_ERR_NULL;

	/* Check if there is anything to save */
	if(g_module_list.next == &g_module_list)
		return SREC_ERR_LIST_EMPTY;

	/* Open the file for writing */
	if((srecord_file = fopen(filename, "wt")) == NULL)
		return SREC_ERR_FILE_OPEN_WRITE;

	/* Go through all the modules */
	for(module = g_module_list.next;module != &g_module_list;module = module->next)
	{
		/* Make an S0 record */
		if((rc = write_s_record(srecord_file, &module->header)) < 0)
		{
			fclose(srecord_file);
			return rc;
		}

		bytes_written += rc;
		for(segment = module->segment_list.next;segment != &module->segment_list;segment = segment->next)
		{
			num_records = 0;
			/* Make the data records */
			for(record = segment->start;record != segment->end->next;record = record->next)
			{
				if((rc = write_s_record(srecord_file, record)) < 0)
				{
					fclose(srecord_file);
					return rc;
				}
				num_records++;
				bytes_written += rc;
			}
			/* Write an S5 record */
			temp_record.type = 5;
			temp_record.address = num_records;
			if((rc = write_s_record(srecord_file, &temp_record)) < 0)
			{
				fclose(srecord_file);
				return rc;
			}
			bytes_written += rc;

			/* Make a termination record */
			temp_record.type = 10 - segment->end->type;
			temp_record.address = segment->execute_address;
			if((rc = write_s_record(srecord_file, &temp_record)) < 0)
			{
				fclose(srecord_file);
				return rc;
			}
			bytes_written += rc;
		}
	}
	fclose(srecord_file);
	return bytes_written;
}


/* Write an S-Record to a file.
 *
 * Data:	file:	The file to write to.
 *			record:	The record to write to file.
 *
 * Return	int:	The number of bytes written or an error message (negative value)
 */
int write_s_record(FILE* file, srec_record* record)
{
	int rc;				/* Return code from encode_s_record() */
	char buff[200];		/* Temporary buffer */
	unsigned int len;	/* length counter */

	/* Sanity check */
	if(file == NULL || record == NULL)
		return SREC_ERR_NULL;

	/* Make an S-Record in buff */
	if((rc = encode_s_record(buff, record)) < 0)
		return rc;
	/* Add a linefeed */
	strcat(buff, "\n");
	len = strlen(buff);
	/* Write to file */
	if(fwrite(buff, 1, len, file) != len)
		return SREC_ERR_FILE_WRITE;

	return len;
}


/* Make an S-Record in a text buffer.
 *
 * Data:	srecord:	The text buffer to write to.
 *			record:	The record to encode.
 *
 * Return:	int:	The number of bytes written or an error message (negative valiue)
 */
int encode_s_record(char* srecord, srec_record* record)
{
	/* Sanity check */
	if(srecord == NULL || record == NULL)
		return SREC_ERR_NULL;

	/* Process based on record type */
	switch(record->type)
	{
		/* These types use a 16-bit address field */
		case 0: case 1: case 9:
			/* Make the head of the record */
			sprintf(srecord, "S%d%02X%04X", record->type, (record->length+3) & 0xff, record->address & 0xffff);
			/* Fill in the data portion */
			hex_to_ascii(record->data, srecord+8, record->length);
			/* Add the checksum to the end */
			sprintf(srecord+(record->length<<1)+8, "%02X", calc_checksum(srecord));
			/* Return the length of the data written */
			return strlen(srecord);
		/* These types used a 24-bit address */
		case 2: case 8:
			sprintf(srecord, "S%d%02X%06X", record->type, (record->length+4) & 0xff, record->address & 0xffffff);
			hex_to_ascii(record->data, srecord+10, record->length);
			sprintf(srecord+(record->length<<1)+10, "%02X", calc_checksum(srecord));
			return strlen(srecord);
		/* These types use a 32-bit address */
		case 3: case 7: case 5:
			sprintf(srecord, "S%d%02X%08X", record->type, (record->length+5) & 0xff, record->address & 0xffffffff);
			hex_to_ascii(record->data, srecord+12, record->length);
			sprintf(srecord+(record->length<<1)+12, "%02X", calc_checksum(srecord));
			return strlen(srecord);
	}
	return SREC_ERR_INVALID_RECORD_TYPE;
}


/* Decode an S-Record and place into a record structure.
 *
 * Data:	srecord:	The S-Record in text form.
 *			record:	The record structure to write to.
 *
 * Return:	int:	The size of the S-Record in text form or an error message (negative value)
 */
int decode_s_record(char* srecord, srec_record* record)
{
	int byte_count;		/* Stores the byte count field of the S-Record */

	/* Sanity check */
	if(srecord == NULL || record == NULL)
		return SREC_ERR_NULL;

	if(*srecord != 'S')
		return SREC_ERR_NOT_S_RECORD;

	/* Check checksum */
	if(calc_checksum(srecord) != (unsigned int)SREC_GET_CHECKSUM(srecord))
		return SREC_ERR_FAILED_CHECKSUM;

	/* Get the byte count */
	byte_count = SREC_GET_BYTE_COUNT(srecord);

	/* Process based on record type */
	switch((record->type = SREC_GET_TYPE(srecord)))
	{
		/* These types used 16-bit addresses */
		case 0: case 1: case 9:
			/* Get the address field */
			record->address = SREC_GET_ADDRESS_16(srecord);
			/* Calculate the length of the data field */
			record->length = byte_count - 3;
			/* Decode the data and store in the record structure */
			ascii_to_hex(srecord+8, record->data, record->length<<1);
			/* Return the size of the S-Record in text form */
			return (byte_count<<1) + 4;
		/* These types use 24-bit addresses */
		case 2: case 8:
			record->address = SREC_GET_ADDRESS_24(srecord);
			record->length = byte_count - 4;
			ascii_to_hex(srecord+10, record->data, record->length<<1);
			return (byte_count<<1) + 4;
		/* These types use 32-bit addresses */
		case 3: case 7: case 5:
			record->address = SREC_GET_ADDRESS_32(srecord);
			record->length = byte_count - 5;
			ascii_to_hex(srecord+12, record->data, record->length<<1);
			return (byte_count<<1) + 4;
	}
	return SREC_ERR_INVALID_RECORD_TYPE;
}


/* Find a module by scanning <number> modules in the linked list.
 *
 * Data:	number:	The module number.
 *
 * Return:	srec_module*:	The module or NULL if not found.
 */
srec_module* get_module_by_number(unsigned int number)
{
	srec_module* module;	/* The current module */

	/* Scan through <number> entries in the list */
	for(module = g_module_list.next;module != &g_module_list;module = module->next)
		if(--number == 0)
			return module;	/* Found it */
	return NULL;	/* Didn't find it */
}


/* Find a segment by scanning n segments in the linked list.
 *
 * Data:	module:	The module whose segment list to search.
 *			number:	The segment number.
 *
 * Return:	srec_segment*:	The segment or NULL if not found.
 */
srec_segment* get_segment_by_number(srec_module* module, unsigned int number)
{
	srec_segment* segment;	/* The current segment */

	/* Scan through <number> segments */
	for(segment = module->segment_list.next;segment != &module->segment_list;segment = segment->next)
		if(--number == 0)
			return segment;	/* Found it */
	return NULL;	/* Didn't find it */
}


/* Make a new module structure and add it to a module list.
 *
 * Data:	list_location:	The location in a module list to add the new module.
 *
 * Return:	srec_module*:	Pointer to the new module or NULL of error.
 */
srec_module* new_module(void)
{
	srec_module* module;	/* Pointer to the new module */

	/* Try to make a new module */
	if((module = malloc(sizeof(srec_module))) == NULL)
		return NULL;

	/* Initialize the module to some default values */
	memset(module, 0, sizeof(*module));
	module->record_list.next = module->record_list.prev = &module->record_list;
	module->segment_list.next = module->segment_list.prev = &module->segment_list;
	module->header.length = 3;
	strcpy(module->header.data, "HDR");

	return module;
}


/* Add a module to a module list.
 *
 * Data:	module:			The module to add.
 *			list_location:	The location in the list to add this module.
 *
 * Return:	int:			0 for success or negative for error.
 */
int add_module(srec_module* module, srec_module* list_location)
{
	/* Sanity check */
	if(module == NULL || list_location == NULL)
		return SREC_ERR_NULL;

	/* Insert into list */
	module->prev = list_location;
	module->next = list_location->next;
	list_location->next->prev = module;
	list_location->next = module;

	return SREC_SUCCESS;
}


/* Remove a module from a list.
 *
 * Data:	module:	The module to remove.
 *
 * Return:	int:	0 for success or negative for error.
 */
int remove_module(srec_module* module)
{
	srec_segment* segment;
	int rc;

	/* Sanity check */
	if(module == NULL)
		return SREC_ERR_NULL;

	if(module->next == NULL || module->prev == NULL)
		return SREC_ERR_NULL;

	if(module->prev == module)
		return SREC_ERR_LIST_EMPTY;

	for(segment = module->segment_list.next;segment != &module->segment_list;segment = segment->next)
		if((rc=delete_segment(segment)) < 0)
			return rc;

	/* Remove from the list */
	module->prev->next = module->next;
	module->next->prev = module->prev;
	module->prev = NULL;
	module->next = NULL;
	return SREC_SUCCESS;
}


/* Delete a module and remove from the list.
 *
 * Data:	module:	The module to delete.
 *
 * Return:	int:	0 for success or negative for error.
 */
int delete_module(srec_module* module)
{
	/* Try to remove it from the list */
	int rc = remove_module(module);
	/* If we succeed, free its memory */
	if(rc >= 0)
		free(module);

	return rc;
}


/* Make a new segment structure and add it to a module list.
 *
 * Data:	module:			The module to add this record to.
 *
 * Return:	srec_segment*:	Pointer to the new segment or NULL of error.
 */
srec_segment* new_segment(void)
{
	srec_segment* segment;	/* Pointer to the new record structure */

	/* Try to make a new record */
	if((segment = malloc(sizeof(srec_segment))) == NULL)
		return NULL;

	/* Clear the record information */
	memset(segment, 0, sizeof(*segment));

	return segment;
}


/* Add a segment to a segment list.
 *
 * Data:	segment:			The segment to add.
 *			list_location:	The location in the list to add this segment.
 *
 * Return:	int:			0 for success or negative for error.
 */
int add_segment(srec_segment* segment, srec_segment* list_location)
{
	/* Sanity check */
	if(segment == NULL || list_location == NULL)
		return SREC_ERR_NULL;

	/* Insert into list */
	segment->prev = list_location;
	segment->next = list_location->next;
	list_location->next->prev = segment;
	list_location->next = segment;

	return SREC_SUCCESS;
}


/* Remove a segment from a list.
 *
 * Data:	segment:	The segment to remove.
 *
 * Return:	int:	0 for success or negative for error.
 */
int remove_segment(srec_segment* segment)
{
	/* Sanity check */
	if(segment == NULL)
		return SREC_ERR_NULL;

	if(segment->next == NULL || segment->prev == NULL)
		return SREC_ERR_NULL;

	if(segment->prev == segment)
		return SREC_ERR_LIST_EMPTY;

	/* Remove from the list */
	segment->prev->next = segment->next;
	segment->next->prev = segment->prev;
	segment->prev = NULL;
	segment->next = NULL;
	return SREC_SUCCESS;
}


/* Delete a segment and remove from the list.
 *
 * Data:	segment:	The segment to delete.
 *
 * Return:	int:	0 for success or negative for error.
 */
int delete_segment(srec_segment* segment)
{
	/* Try to remove it from the list */
	int rc = remove_segment(segment);
	/* If we succeed, free its memory */
	if(rc >= 0)
		free(segment);

	return rc;
}


/* Make a new record structure and add it to a module list.
 *
 * Data:	module:			The module to add this record to.
 *
 * Return:	srec_record*:	Pointer to the new record or NULL of error.
 */
srec_record* new_record()
{
	srec_record* record;	/* Pointer to the new record structure */

	/* Try to make a new record */
	if((record = malloc(sizeof(srec_record))) == NULL)
		return NULL;

	/* Clear the record information */
	memset(record, 0, sizeof(*record));

	return record;
}


/* Add a record to a record list.
 *
 * Data:	record:			The record to add.
 *			list_location:	The location in the list to add this record.
 *
 * Return:	int:			0 for success or negative for error.
 */
int add_record(srec_record* record, srec_record* list_location)
{
	/* Sanity check */
	if(record == NULL || list_location == NULL)
		return SREC_ERR_NULL;

	/* Insert into list */
	record->prev = list_location;
	record->next = list_location->next;
	list_location->next->prev = record;
	list_location->next = record;

	return SREC_SUCCESS;
}


/* Remove a record from a list.
 *
 * Data:	record:	The record to remove.
 *
 * Return:	int:	0 for success or negative for error.
 */
int remove_record(srec_record* record)
{
	/* Sanity check */
	if(record == NULL)
		return SREC_ERR_NULL;
	if(record->next == NULL || record->prev == NULL)
		return SREC_ERR_NULL;
	if(record->prev == record)
		return SREC_ERR_LIST_EMPTY;
	/* Remove from the list */
	record->prev->next = record->next;
	record->next->prev = record->prev;
	record->prev = NULL;
	record->next = NULL;
	return SREC_SUCCESS;
}


/* Delete a record and remove from the list.
 *
 * Data:	record:	The record to delete.
 *
 * Return:	int:	0 for success or negative for error.
 */
int delete_record(srec_record* record)
{
	/* Try to remove it from the list */
	int rc;
	rc = remove_record(record);
	/* If we succeed, free its memory */
	if(rc >= 0)
		free(record);
	return rc;
}


/* Count the length of whitespace until non-whitepace is encountered or until
 * we reach the end of the buffer.
 *
 * Data:	buff:			The buffer to examine.
 *			length:			The length to search.
 *
 * Return:	usigned int:	The number of bytes worth of whitespace.
 */
unsigned int get_whitespace_length(char* buff, unsigned int length)
{
	unsigned int i;	/* Counter */

	if(buff == NULL)
		return 0;

	/* Look for CR, LF, tabs and spaces */
	for(i=0;i<length;i++)
		if(buff[i] != '\r' &&
		   buff[i] != '\n' &&
		   buff[i] != '\t' &&
		   buff[i] != ' ')
			break;
	return i;
}


/* Calculate the checksum of an s-record.
 *
 * Data:	s_record:		The s-record to examine.
 *
 * Return:	unsigned int:	The checksum of this S-Record.
 */
unsigned int calc_checksum(char* s_record)
{
	unsigned int checksum = 0;	/* Used to calculate the checksum */
	unsigned int i;				/* Counter */
	/* The s-record's byte count */
	unsigned int length = SREC_GET_BYTE_COUNT(s_record) << 1;
	/* Move to the part of the s-record that is used in checksum calculation */
	s_record += 2;

	/* Sum all the data */
	for(i=0;i<length;i+=2)
		checksum += GET_HEX_8BIT(s_record+i);
	/* Return the 1's complement of the sum */
	return (~checksum) & 0xff;
}


/* Convert ASCII representation of hex to binary
 *
 * Data:	ascii:			The ascii representation of the data.
 *			binary:			The binary representation of the data.
 *			ascii_length:	Length of the ascii buffer.
 *
 * Return:	void
 */
void ascii_to_hex(char* ascii, char* binary, unsigned int ascii_length)
{
	/* Get the end of our buffer */
	char* ascii_end = ascii + ascii_length;

	/* Convert 2 ASCII chars into 1 binary char */
	for(;ascii<ascii_end;ascii+=2, binary++)
		*binary = GET_HEX_8BIT(ascii);
}


/* Convert binary data to ASCII representation of hex
 * Data:	binary:			The binary representation of the data.
 *			ascii:			The ascii representation of the data.
 *			binary_length:	Length of the binary buffer.
 *
 * Return:	void
 */
void hex_to_ascii(char* binary, char* ascii, unsigned int binary_length)
{
	/* Get the end of our buffer */
	char* binary_end = binary + binary_length;

	/* Convert 1 binary char into 2 ASCII chars */
	for(;binary<binary_end;binary++, ascii+=2)
		sprintf(ascii, "%02X", *binary);
}


/* Print the list fo contents of the S-Record to stdout.
 * Data:	void
 *
 * Return:	void
 */
void list_srecords(void)
{
	srec_module* module;			/* current module */
	srec_segment* segment;			/* current segment */
	unsigned int module_num = 1;	/* current module number */
	unsigned int first_in_list;		/* flag if this is the first segment in the list */
	unsigned int segment_num;		/* current segment number */

	/* Print the header */
	printf("Mod Seg Description                                T    Start      End  Execute\n");
	printf("--- --- ------------------------------------------ - -------- -------- --------\n");

	/* Go through all the modules */
	for(module = g_module_list.next;module != &g_module_list;module = module->next)
	{
		first_in_list = 1;
		segment_num = 1;
		/* Go through each segment */
		for(segment = module->segment_list.next;segment != &module->segment_list;segment = segment->next)
		{
			/* If this is the first segment of this module, print the module info as well */
			if(first_in_list)
			{
				if(module->header.length > 42)
					printf("%3d %3d %-41.41s> %d %8X %8X %8X\n", module_num, segment_num, module->header.data, segment->start->type, segment->start->address, segment->end->address, segment->execute_address);
				else
					printf("%3d %3d %-42.42s %d %8X %8X %8X\n", module_num, segment_num, module->header.data, segment->start->type, segment->start->address, segment->end->address, segment->execute_address);
				first_in_list = 0;
			}
			/* Otherwise just print the necessary details */
			else
				printf("    %3d %-42.42s %d %8X %8X %8X\n", segment_num, "", segment->start->type, segment->start->address, segment->end->address, segment->execute_address);
			segment_num++;
		}
		module_num++;
	}
}


/* List of errors with string descriptions */
char* srec_errors[] =
{
	"Unknown Error",							/* 0 */
	"Function was called with a NULL pointer",	/* -1 */
	"Invalid record type",						/* -2 */
	"Not an S record",							/* -3 */
	"Data length too long",						/* -4 */
	"Failed checksum",							/* -5 */
	"Could not open file for reading",			/* -6 */
	"Could not open file for writing",			/* -7 */
	"Could not read from file",					/* -8 */
	"Could not write to file",					/* -9 */
	"Could not allocate memory",				/* -10 */
	"Tried to access an empty list",			/* -11 */
	"Bad record count",							/* -12 */
	"Extra terminator record",					/* -13 */
	"Module does not exist",					/* -14 */
	"Segment does not exist",					/* -15 */
};


/* Useage messages */

char* useage_messages[] =
{
	"srecord a <srecord file> <file> [name=<module name> | module=<module number>] [address=<address>] [length=<record length>] [type=<record type>] [execute=<execute address>]",
	"srecord e <srecord file> <module number> <segment number> <filename>",
	"srecord p <srecord file> <module number> <segment number>",
	"srecord d <srecord file> <module number> <segment number>",
};

void print_useage(void)
{
	printf("SRecord v1.0 by Karl Stenerud\n");
	printf("S-Record management program\n\n");
	printf("Usage: srecord <s-record file> <options>\n\n");
	printf("Valid options:\n");
	printf("\t%s\n", useage_messages[0]);
	printf("\t%s\n", useage_messages[1]);
	printf("\t%s\n", useage_messages[2]);
	printf("\t%s\n", useage_messages[3]);
	exit(1);
}

void print_usage_add()
{
	printf("Useage: %s\n", useage_messages[0]);
	exit(1);
}

void print_usage_extract()
{
	printf("Useage: %s\n", useage_messages[1]);
	exit(1);
}

void print_usage_print()
{
	printf("Useage: %s\n", useage_messages[2]);
	exit(1);
}

void print_usage_delete()
{
	printf("Useage: %s\n", useage_messages[3]);
	exit(1);
}


int main(int argc, char* argv[])
{
	int rc;						/* Return code from function calls */
	char* operation = argv[1];	/* Operation to perform */
	char* archive_filename = argv[2];	/* name of archive file */
	char* binary_filename;		/* Name of binary file */
	char* name;					/* Name when creating new module */
	unsigned int length;		/* Length */
	unsigned int address;		/* Load address */
	unsigned int type;			/* Record type */
	unsigned int execute;		/* Execute address */
	unsigned int module_num;	/* Module number */
	unsigned int segment_num;	/* Segment number */
	int i;						/* Counter */
	srec_module* module;		/* Current module */
	srec_segment* segment;		/* Current segment */

	if(argc < 2)
		print_useage();

	/* Process based on the operation */
	switch(*operation)
	{
		case 'l':	/* List data in S-Record file */
			if((rc=read_srecord_file(archive_filename)) <0)
			{
				printf("Error %d reading from %s: %s\n", rc, archive_filename, srec_errors[-rc]);
				exit(-1);
			}
			printf("\nContents of S-Record file %s:\n\n", archive_filename);
			list_srecords();
			break;
		case 'a':	/* Add binary file to S-Record file */
			// srecord a <srecord file> <file> [name=<module name>] [address=<address>] [length=<record length>] [type=<record type>] [execute=<execute address>]

			if(argc < 4)
				print_usage_add();

			/* Some defaults */
			binary_filename = argv[3];
			name = binary_filename;
			address = 0;
			length = 30;
			type = 3;
			execute = 0;
			module_num = 0;

			/* Decode options */
			for(i=4;i<argc;i++)
			{
				if(strncmp(argv[i], "name=", 5) == 0)
					name = argv[i]+5;
				else if(strncmp(argv[i], "address=", 8) == 0)
					sscanf(argv[i]+8, "%X", &address);
				else if(strncmp(argv[i], "length=", 7) == 0)
					length = atoi(argv[i]+7);
				else if(strncmp(argv[i], "type=", 5) == 0)
					type = atoi(argv[i]+5);
				else if(strncmp(argv[i], "execute=", 8) == 0)
					sscanf(argv[i]+8, "%X", &execute);
				else if(strncmp(argv[i], "module=", 7) == 0)
					module_num = atoi(argv[i]+7);
				else
				{
					printf("%s: invalid option.\n", argv[i]);
					exit(-1);
				}
			}

			/* Read in the S-Redord file */
			read_srecord_file(archive_filename);

			/* Add the binary file */
			if((rc=add_binary_file(binary_filename, name, address, length, type, execute, module_num)) < 0)
			{
				printf("Error %d reading from %s: %s\n", rc, binary_filename, srec_errors[-rc]);
				exit(-1);
			}
			/* Write the new S-Record file */
			if((rc=write_srecord_file(archive_filename)) <0)
			{
				printf("Error %d writing to %s: %s\n", rc, archive_filename, srec_errors[-rc]);
				exit(-1);
			}
			break;
		case 'e':	/* Extract a segment and save as a binary file */
			// srecord e <archive> <module number> <segment number> <filename>
			if(argc < 6)
				print_usage_extract();

			module_num = atoi(argv[3]);
			segment_num = atoi(argv[4]);
			binary_filename = argv[5];

			if((rc=read_srecord_file(archive_filename)) <0)
			{
				printf("Error %d reading from %s: %s\n", rc, archive_filename, srec_errors[-rc]);
				exit(-1);
			}
			if((rc=write_binary_file(module_num, segment_num, binary_filename)) < 0)
			{
				printf("Error %d trying to save %s: %s\n", rc, binary_filename, srec_errors[-rc]);
				exit(-1);
			}
			break;
		case 'p':	/* Print the contents of a segment to stdout */
			// srecord p <archive> <module number> <segment number>
			if(argc < 5)
				print_usage_print();

			module_num = atoi(argv[3]);
			segment_num = atoi(argv[4]);

			if((rc=read_srecord_file(archive_filename)) <0)
			{
				printf("Error %d reading from %s: %s\n", rc, archive_filename, srec_errors[-rc]);
				exit(-1);
			}
			if((rc=write_binary_file(module_num, segment_num, NULL)) < 0)
			{
				printf("Error %d trying to print to screen: %s\n", rc, srec_errors[-rc]);
				exit(-1);
			}
			break;
		case 'd':	/* Delete a segment or module from the S-Record file */
			// srecord d <archive> <module number> <segment number>
			if(argc < 5)
				print_usage_delete();

			module_num = atoi(argv[3]);
			segment_num = atoi(argv[4]);

			if((rc=read_srecord_file(archive_filename)) <0)
			{
				printf("Error %d reading from %s: %s\n", rc, archive_filename, srec_errors[-rc]);
				exit(-1);
			}
			if((module = get_module_by_number(module_num)) == NULL)
			{
				printf("Invalid module number\n");
				exit(-1);
			}
			if((segment = get_segment_by_number(module, segment_num)) == NULL)
			{
				printf("Invalid segment number\n");
				exit(-1);
			}
			if((rc=delete_segment(segment)) < 0)
			{
				printf("Error deleting segment %d of module %d: %d\n", segment_num, module_num, rc);
				exit(-1);
			}
			if((rc=write_srecord_file(archive_filename)) <0)
			{
				printf("Error %d writing to %s: %s\n", rc, archive_filename, srec_errors[-rc]);
				exit(-1);
			}

	}
	return 0;
}
