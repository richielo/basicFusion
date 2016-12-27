#ifndef TERRA_H
#define TERRA_H
#include <hdf.h>
#include <mfhdf.h>
#include <hdf5.h>
#include <hdf5_hl.h>

#define DIM_MAX 10

/*********************
 *FUNCTION PROTOTYPES*
 *********************/
extern hid_t outputFile;

int MOPITT( char* argv[] );
int CERES( char* argv[] ,int index);
int MODIS( char* argv[],int modis_count,int unpack );
//int MODIS( char* argv[]);
int ASTER( char* argv[],int aster_count,int unpack );
//int ASTER( char* argv[]);
int MISR( char* argv[],int unpack );

hid_t insertDataset( hid_t const *outputFileID, hid_t *datasetGroup_ID, 
					 int returnDatasetID, int rank, hsize_t* datasetDims, 
					 hid_t dataType, char* datasetName, void* data_out);
					 
hid_t MOPITTinsertDataset( hid_t const *inputFileID, hid_t *datasetGroup_ID, 
							char * inDatasetPath, char* outDatasetPath, hid_t dataType, int returnDatasetID);
herr_t openFile(hid_t *file, char* inputFileName, unsigned flags );
herr_t createOutputFile( hid_t *outputFile, char* outputFileName);
herr_t createGroup( hid_t const *referenceGroup, hid_t *newGroup, char* newGroupName);
/* general type attribute creation */
hid_t attributeCreate( hid_t objectID, const char* attrName, hid_t datatypeID );
/* creates and writes a string attribute */
hid_t attrCreateString( hid_t objectID, char* name, char* value );

int32 H4ObtainLoneVgroupRef(int32 file_id, char *groupname);

int32 H4readData( int32 fileID, char* datasetName, void** data,
				  int32 *rank, int32* dimsizes, int32 dataType );
hid_t readThenWrite( hid_t outputGroupID, char* datasetName, int32 inputDataType, 
					   hid_t outputDataType, int32 inputFile);	  

char *correct_name(const char* oldname);

/* ASTER functions */

hid_t readThenWrite_ASTER_Unpack( hid_t outputGroupID, char* datasetName, int32 inputDataType,
                                           hid_t outputDataType, int32 inputFile, float unc);

/* MISR funcions */
hid_t readThenWrite_MISR_Unpack( hid_t outputGroupID, char* datasetName, int32 inputDataType,
                                           hid_t outputDataType, int32 inputFile, float scale_factor);

hid_t readThenWrite_MODIS_Unpack( hid_t outputGroupID, char* datasetName, int32 inputDataType,
                                  hid_t outputDataType, int32 inputFileID);

hid_t readThenWrite_MODIS_Uncert_Unpack( hid_t outputGroupID, char* datasetName, int32 inputDataType,
                                  hid_t outputDataType, int32 inputFileID);


#if 0
float unc[5][15] =
{
{0.676,0.708,0.423,0.423,0.1087,0.0348,0.0313,0.0299,0.0209,0.0159,-1,-1,-1,-1,-1},
{1.688,1.415,0.862,0.862,0.2174,0.0696,0.0625,0.0597,0.0417,0.0318,0.006822,0.006780,0.006590,0.005693,0.005225},
{2.25,1.89,1.15,1.15,0.290,0.0925,0.0830,0.0795,0.0556,0.0424,-1,-1,-1,-1,-1},
{-1,-1,-1,-1,0.290,0.409,0.390,0.332,0.245,0.265,-1,-1,-1,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
};

const char* metadata_gain="productmetadata.0";
const char *band_index_str_list[10] = {"01","02","3N","3B","04","05","06","07","08","09"};
const char *gain_stat_str_list[5]  ={"HGH","NOR","LO1","LO2","OFF"};


char* obtain_gain_info(char *whole_string);
short get_band_index(char *band_index_str);
short get_gain_stat(char *gain_stat_str);
#endif





#endif