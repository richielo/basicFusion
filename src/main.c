#include "libTERRA.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <curses.h>
#define STR_LEN 500
#define LOGIN_NODE "login"
#define MOM_NODE



int getNextLine ( char* string, FILE* const inputFile );
herr_t updateGranList( char** granList, const char newGran[], size_t* curSize );

int main( int argc, char* argv[] )
{

    /* Check that this program is not being run on a login node or a MOM node.
     * If it is, shame on you!
     */

#if 0
    {
        const char* hostname = getenv("HOSTNAME");
        printf("%s", hostname );
        if ( strstr( hostname, LOGIN_NODE ) != NULL )
        {
            fprintf( stderr, "\n\t\x1b[1m\x1b[31m\x1b[47m*******************\n");
            fprintf( stderr, "\t* \x1b[4mCRITICAL ERROR\x1b[24m! *\n");
            fprintf( stderr, "\t*******************\x1b[0m\n\n");
            fprintf( stderr, "You have attempted to run this program on a non-compute node!\n");
            fprintf( stderr, "Don't ever do such a thing! Please refer to README.txt in the root project directory.\n\n");
            return EXIT_FAILURE;
        }

    }
#endif

    char* MOPITTargs[3] = {NULL};
    char* CERESargs[4] = {NULL};
    char* MODISargs[7] = {NULL};
    char* ASTERargs[4] = {NULL};
    char* MISRargs[13] = {NULL};

    char* granuleList = NULL;
    size_t granListSize = 0;
    char* granTempPtr = NULL;
    int status = EXIT_SUCCESS;
    int fail = 0;
    herr_t errStatus;

    /* Main will assert that the lines found in the inputFiles.txt file match
     * the expected input files. This will be done using string.h's strstr() function.
     * This will halt the execution of the program if the line recieved by getNextLine
     * does not match the expected input. In other words, only input files meant for
     * MOPITT, CERES, ASTER etc will pass this error check.
     */
    char MOPITTcheck[] = "MOP01";
    char CEREScheck1[] = "CER_SSF_Terra-FM1";

    /*MY 2016-12-20: add FM2 based on GZ's request.  */
    char CEREScheck2[] = "CER_SSF_Terra-FM2";
    char MODIScheck1[] = "MOD021KM";
    char MODIScheck2[] = "MOD02HKM";
    char MODIScheck3[] = "MOD02QKM";
    char MODIScheck4[] = "MOD03";
    char ASTERcheck[] = "AST_L1T_";
    char MISRcheck1[] = "MISR_AM1_GRP";
    char MISRcheck2[] = "MISR_AM1_AGP";


    /* MY 2016-12-21: this is added on GZ's requet. This is for solar geometry */
    char MISRcheck3[] = "MISR_AM1_GP";

    /* MY 2017-03-04: Add high-resolution Lat/lon  */
    char MISRcheck4[] = "MISR_HRLL";

    /* MY 2016-12-21: granule is added on GZ's request. Basically per time per granule for ASTER and MODIS.*/

    char granule[] ="granule";
    char modis_granule_suffix[3] = {'\0'};
    char aster_granule_suffix[3] = {'\0'};
    char ceres_granule_suffix[3] = {'\0'};


    memset(modis_granule_suffix,0,3);
    memset(aster_granule_suffix,0,3);
    memset(ceres_granule_suffix,0,3);

    /*MY 2016-12-21: MODIS and ASTER have more than one granule for each orbit. We use a counter to stop the loop. */
    int ceres_count = 1;
    int ceres_fm1_count = 1;
    int ceres_fm2_count = 1;
    int ceres_start_index = 0;
    int* ceres_start_index_ptr=&ceres_start_index;
    int ceres_end_index = 0;
    int* ceres_end_index_ptr=&ceres_end_index;
    int ceres_subset_num_elems = 0;
    int32* ceres_subset_num_elems_ptr=NULL;
    int modis_count = 1;
    int aster_count = 1;

    OInfo_t current_orbit_info;
    OInfo_t* test_orbit_ptr = NULL;

    FILE* inputFile = NULL;
    char string[STR_LEN];

    /*MY 2016-12-21: GZ also requests to unpack the MODIS,ASTER and MISR data, this is the flag for this */
    int unpack      = 1;

    if ( argc != 4 )
    {
        fprintf( stderr, "Usage: %s [outputFile] [inputFiles.txt] [orbit_info.bin]\n", argv[0] );
        fprintf( stderr, "Set environment variable TERRA_DATA_UNPACK to non-zero for unpacking.\n");
        goto cleanupFail;
    }

    /* open the input file list */
    inputFile = fopen( argv[2], "r" );

    if ( inputFile == NULL )
    {
        FATAL_MSG("file \"%s\" does not exist. Exiting program.\n", argv[2]);
        goto cleanupFail;
    }

    // open the orbit_info.bin file
    FILE* new_orbit_info_b = fopen(argv[3],"r");
    long fSize;

    // get the size of the file
    fseek(new_orbit_info_b,0,SEEK_END);
    fSize = ftell(new_orbit_info_b);
    // rewind file pointer to beginning of file
    rewind(new_orbit_info_b);


    test_orbit_ptr = calloc(fSize/sizeof(OInfo_t),sizeof(OInfo_t));

    // read the file into the test_orbit_ptr struct array
    size_t result = fread(test_orbit_ptr,sizeof(OInfo_t),fSize/sizeof(OInfo_t),new_orbit_info_b);
    if(result != fSize/sizeof(OInfo_t))
    {
        FATAL_MSG("fread is not successful.\n");
        goto cleanupFail;
    }


    /* Get the orbit number from inputFiles.txt */
    status = getNextLine( string, inputFile );
    if ( status == EXIT_FAILURE )
    {
        FATAL_MSG("Failed to get the next line.\n");
        goto cleanupFail;
    }
    if ( !isdigit(*string) )
    {
        FATAL_MSG("The first line in the %s file must contain the orbit number.\n", argv[2]);
        goto cleanupFail;
    }

    int current_orbit_number = atoi(string);
    for ( int i = 0; i<fSize/sizeof(OInfo_t); i++)
    {
        if( test_orbit_ptr[i].orbit_number == current_orbit_number )
        {
            current_orbit_info = test_orbit_ptr[i];
            break;
        }
    }

    fclose(new_orbit_info_b);
    new_orbit_info_b = NULL;
    free(test_orbit_ptr);
    test_orbit_ptr = NULL;



    /*MY 2016-12-21: Currently an environment variable TERRA_DATA_UNPACK should be set
     *to run the program to generate the unpacked data,
     *without setting this environment variable, the program will just write the packed data */

    {
        const char *s;
        s = getenv("TERRA_DATA_PACK");

        if(s && isdigit((int)*s))
            if((unsigned int)strtol(s,NULL,10) == 1)
                unpack = 0;
    }

    if ( unpack ) printf("\n_____UNPACKING ENABLED_____\n");
    else printf("\n_____UNPACKING DISABLED_____\n");


    /* remove output file if it already exists. Note that no conditional statements are used. If file does not exist,
     * this function will throw an error but we do not care.
     */
    /* MY 2017-01-27: remove is not necessary. The HDF5 file create operation will handle this
                      Correction: leave it for the time being since the createOutputFile uses the EXCL flag.
                      TODO: Will turn this off in the operation.
    */
    remove( argv[1] );

    /* create the output file or open it if it exists */
    if ( createOutputFile( &outputFile, argv[1] ))
    {
        FATAL_MSG("Unable to create output file.\n");
        outputFile = 0;
        goto cleanupFail;
    }


    /**********
     * MOPITT *
     **********/

    printf("Transferring MOPITT...");
    fflush(stdout);
    /* get the MOPITT input file paths from our inputFiles.txt */
    status = getNextLine( string, inputFile);

    if ( status == EXIT_FAILURE )
    {
        FATAL_MSG("Failed to get MOPITT line. Exiting program.\n");
        goto cleanupFail;
    }

    /* strrchr finds last occurance of character in a string */
    granTempPtr = strrchr( string, '/' );
    if ( granTempPtr == NULL )
    {
        FATAL_MSG("Failed to find the last occurance of slash character in the input line.\n");
        goto cleanupFail;
    }
    errStatus = updateGranList( &granuleList, granTempPtr + 1, &granListSize );
    if ( errStatus == EXIT_FAILURE )
    {
        FATAL_MSG("Failed to update the granule list.\n");
        goto cleanupFail;
    }

    /* strcmp returns 0 if the strings match */
    if ( strcmp(string, "MOP N/A" ) )
    {
        int MOPITTgranule = 1;

        // minor: strstr may be called twice in the loop. No need to change. KY 2017-04-10
        do
        {
            /* strstr will fail if unexpected input is present */
            if ( strstr( string, MOPITTcheck ) == NULL )
            {
                FATAL_MSG("Received an unexpected input line for MOPITT.\n\tReceived string:\n\t%s\n\tExpected to receive string containing a substring of: %s\nExiting program.\n",string,MOPITTcheck);
                goto cleanupFail;
            }

            MOPITTargs[0] = argv[0];

            /* allocate space for the argument */
            MOPITTargs[1] = malloc( strlen(string)+1 );
            memset(MOPITTargs[1],0,strlen(string)+1);

            /* copy the data over */
            strncpy( MOPITTargs[1], string, strlen(string) );
            MOPITTargs[2] = argv[1];

            if ( MOPITT( MOPITTargs, current_orbit_info, &MOPITTgranule ) == EXIT_FAILURE )
            {
                FATAL_MSG("MOPITT failed data transfer.\nExiting program.\n");
                goto cleanupFail;
            }

            free(MOPITTargs[1]);
            MOPITTargs[1] = NULL;

            status = getNextLine( string, inputFile);

            if ( status == EXIT_FAILURE )
            {
                FATAL_MSG("Failed to get MOPITT line. Exiting program.\n");
                goto cleanupFail;
            }

            
            granTempPtr = strrchr( string, '/' );
            if ( granTempPtr == NULL )
            {
                FATAL_MSG("Failed to find the last occurance of slash character in the input line.\n");
                goto cleanupFail;
            }
            errStatus = updateGranList( &granuleList, granTempPtr + 1, &granListSize );
            if ( errStatus == EXIT_FAILURE )
            {
                FATAL_MSG("Failed to update the granule list.\n");
                goto cleanupFail;
            }
            /* LTC Apr-7-2017 Note that if the current granule was skipped, MOPITTgranule will be decremented by MOPITT.c
               so that it contains the correct running total for number of granules that have been read.*/
            MOPITTgranule++;

        } while ( strstr( string, MOPITTcheck ) != NULL );


        printf("MOPITT done.\nTransferring CERES...");
        fflush(stdout);
    }
    else
    {
        printf("No files for MOPITT found.\nTransferring CERES...");
        fflush(stdout);
        status = getNextLine( string, inputFile);
        if ( status == EXIT_FAILURE )
        {
            FATAL_MSG("Failed to get next line. Exiting program.\n");
            goto cleanupFail;
        }
    
        
        granTempPtr = strrchr( string, '/' );
        if ( granTempPtr == NULL )
        {
            FATAL_MSG("Failed to find the last occurance of slash character in the input line.\n");
            goto cleanupFail;
        }
        errStatus = updateGranList( &granuleList, granTempPtr + 1, &granListSize );
        if ( errStatus == EXIT_FAILURE )
        {
            FATAL_MSG("Failed to update the granule list.\n");
            goto cleanupFail;
        }
    }

    /*********
     * CERES *
     *********/
    /* Get the CERES  */
#if 0
    status = getNextLine( string, inputFile);
    if ( status == EXIT_FAILURE )
    {
        FATAL_MSG("Failed to get CERES line. Exiting program.\n");
        goto cleanupFail;
    }


    if ( strstr( string, CEREScheck1 ) == NULL )
    {
        FATAL_MSG("Received an unexpected input line for CERES.\n\tReceived string:\n\t%s\n\tExpected to receive string containing a substring of: %s\nExiting program.\n", string, CEREScheck1);
        goto cleanupFail;
    }

    CERESargs[0] = argv[0];

    /* Allocate memory for the argument */
    CERESargs[1] = malloc( strlen(string )+1);
    memset(CERESargs[1],0,strlen(string)+1);
    strncpy( CERESargs[1], string, strlen(string) );
    CERESargs[2] = argv[1];

    if ( CERES( CERESargs,1) == EXIT_FAILURE )
    {
        FATAL_MSG("CERES failed data transfer.\nExiting program.\n");
        goto cleanupFail;
    }


    free(CERESargs[1]);
    CERESargs[1] = NULL;

    // Handler CERES FM2
    getNextLine( string, inputFile);
    if ( strstr( string, CEREScheck2 ) == NULL )
    {
        FATAL_MSG("Failed to get CERES line.\nExpected: %s\nRecieved: %s\n", CEREScheck2, string);
        goto cleanupFail;
    }

    CERESargs[0] = argv[0];

    /* Allocate memory for the argument */
    CERESargs[1] = malloc( strlen(string )+1);
    memset(CERESargs[1],0,strlen(string)+1);
    strncpy( CERESargs[1], string, strlen(string) );
    CERESargs[2] = argv[1];

    if ( CERES( CERESargs,2) == EXIT_FAILURE )
    {
        FATAL_MSG("CERES failed data transfer.\nExiting program.\n");
        goto cleanupFail;
    }
    free(CERESargs[1]);
    CERESargs[1] = NULL;


    printf("CERES done.\nTransferring MODIS...");
    fflush(stdout);

#endif

    CERESargs[0] = argv[0];
    CERESargs[1] = argv[1];


    if ( strcmp(string, "CER N/A" ) )
    {
        while(ceres_count != 0)
        {


            if(strstr(string,MODIScheck1) != NULL)
            {
                ceres_count = 0;
                continue;
            }

            if ( strstr( string, CEREScheck1 ) != NULL )
            {
                CERESargs[2] = malloc(strlen(string)+1);
                memset(CERESargs[2],0,strlen(string)+1);
                strncpy( CERESargs[2], string, strlen(string) );
                //FM1
                status = CERES_OrbitInfo(CERESargs,ceres_start_index_ptr,ceres_end_index_ptr,current_orbit_info);
                if ( status == EXIT_FAILURE )
                {
                    FATAL_MSG("CERES failed to obtain orbit info.\nExiting program.\n");
                    goto cleanupFail;
                }

                if(*ceres_start_index_ptr >=0 && *ceres_end_index_ptr >=0)
                {
                    /* Remember the granule number */
                    sprintf(ceres_granule_suffix,"%d",ceres_fm1_count);
                    CERESargs[3] = malloc(strlen(granule)+strlen(ceres_granule_suffix)+1);
                    memset(CERESargs[3],0,strlen(granule)+strlen(ceres_granule_suffix)+1);

                    strncpy(CERESargs[3],granule,strlen(granule));
                    strncat(CERESargs[3],ceres_granule_suffix,strlen(ceres_granule_suffix));

                    ceres_subset_num_elems = (*ceres_end_index_ptr) -(*ceres_start_index_ptr)+1;
                    ceres_subset_num_elems_ptr =&ceres_subset_num_elems;
                    status = CERES(CERESargs,1,ceres_fm1_count,(int32*)ceres_start_index_ptr,NULL,ceres_subset_num_elems_ptr);
                    if ( status == EXIT_FAILURE )
                    {
                        FATAL_MSG("CERES failed data transfer.\nExiting program.\n");
                        goto cleanupFail;
                    }
                    ceres_fm1_count++;
                }

            }
            else if ( strstr( string, CEREScheck2 ) != NULL )
            {
                CERESargs[2] = malloc(strlen(string)+1);
                memset(CERESargs[2],0,strlen(string)+1);
                strncpy( CERESargs[2], string, strlen(string) );
                //FM2
                status = CERES_OrbitInfo(CERESargs,ceres_start_index_ptr,ceres_end_index_ptr,current_orbit_info);
                if ( status == EXIT_FAILURE )
                {
                    FATAL_MSG("CERES failed to obtain orbit info.\nExiting program.\n");
                    goto cleanupFail;
                }

                if(*ceres_start_index_ptr >=0 && *ceres_end_index_ptr >=0)
                {
                    sprintf(ceres_granule_suffix,"%d",ceres_fm2_count);
                    CERESargs[3] = malloc(strlen(granule)+strlen(ceres_granule_suffix)+1);
                    memset(CERESargs[3],0,strlen(granule)+strlen(ceres_granule_suffix)+1);

                    strncpy(CERESargs[3],granule,strlen(granule));
                    strncat(CERESargs[3],ceres_granule_suffix,strlen(ceres_granule_suffix));

                    ceres_subset_num_elems = (*ceres_end_index_ptr) -(*ceres_start_index_ptr)+1;
                    ceres_subset_num_elems_ptr =&ceres_subset_num_elems;
                    status = CERES(CERESargs,2,ceres_fm2_count,(int32*)ceres_start_index_ptr,NULL,ceres_subset_num_elems_ptr);
                    //status = CERES_Subset(CERESargs,1,ceres_fm1_count,unpack,ceres_start_index,ceres_end_index);
                    if ( status == EXIT_FAILURE )
                    {
                        FATAL_MSG("CERES failed data transfer.\nExiting program.\n");
                        goto cleanupFail;
                    }
                    ceres_fm2_count++;
                }


            }
            else
            {
                FATAL_MSG("CERES files are neither CER_SSF_Terra-FM1 nor CER_SSF_Terra-FM2.\n");
                goto cleanupFail;
            }

            status = getNextLine( string, inputFile);
            if ( status == EXIT_FAILURE )
            {
                FATAL_MSG("Failed to get next line. Exiting program.\n");
                goto cleanupFail;
            }

            
            granTempPtr = strrchr( string, '/' );
            if ( granTempPtr == NULL )
            {
                FATAL_MSG("Failed to find the last occurance of slash character in the input line.\n");
                goto cleanupFail;
            }
            errStatus = updateGranList( &granuleList, granTempPtr + 1, &granListSize );
            if ( errStatus == EXIT_FAILURE )
            {
                FATAL_MSG("Failed to update the granule list.\n");
                goto cleanupFail;
            }

            if(CERESargs[2]) free(CERESargs[2]);
            CERESargs[2] = NULL;
            if(CERESargs[3]) free(CERESargs[3]);
            CERESargs[3] = NULL;
            ceres_count++;
        }

        printf("CERES done.\nTransferring MODIS...");
        fflush(stdout);
    }
    else
    {
        printf("No CERES files found.\nTransferring MODIS...");
        fflush(stdout);
        status = getNextLine( string, inputFile);
        if ( status == EXIT_FAILURE )
        {
            FATAL_MSG("Failed to get next line. Exiting program.\n");
            goto cleanupFail;
        }

        
        granTempPtr = strrchr( string, '/' );
        if ( granTempPtr == NULL )
        {
            FATAL_MSG("Failed to find the last occurance of slash character in the input line.\n");
            goto cleanupFail;
        }
        errStatus = updateGranList( &granuleList, granTempPtr + 1, &granListSize );
        if ( errStatus == EXIT_FAILURE )
        {
            FATAL_MSG("Failed to update the granule list.\n");
            goto cleanupFail;
        }
    }

    /*********
     * MODIS *
     *********/


    MODISargs[0] = argv[0];
    MODISargs[6] = argv[1];

    /* MY 2016-12-21: Need to form a loop to read various number of MODIS files
    *  A pre-processing check of the inputFile should be provided to make sure the processing
    *  will not exit prematurely */
    if ( strcmp(string, "MOD N/A" ) )
    {
        while(modis_count != 0)
        {


            /* Need to check more, now just assume ASTER is after */
            if(strstr(string,ASTERcheck)!=NULL)
            {
                modis_count = 0;
                continue;
            }

            /* Should be 1 km */
            if ( strstr( string, MODIScheck1 ) == NULL )
            {
                FATAL_MSG("Received an unexpected input line for MODIS.\n\tReceived string:\n\t%s\n\tExpected to receive string containing a substring of: %s\nExiting program.\n", string, MODIScheck1);
                goto cleanupFail;
            }


            /* Allocate memory for the argument */
            MODISargs[1] = malloc ( strlen(string )+1 );
            memset(MODISargs[1],0,strlen(string)+1);
            strncpy( MODISargs[1], string, strlen(string));

            status = getNextLine( string, inputFile );
            if ( status == EXIT_FAILURE )
            {
                FATAL_MSG("Failed to get MODIS line. Exiting program.\n");
                goto cleanupFail;
            }
            
            
            granTempPtr = strrchr( string, '/' );
            if ( granTempPtr == NULL )
            {
                FATAL_MSG("Failed to find the last occurance of slash character in the input line.\n");
                goto cleanupFail;
            }
            errStatus = updateGranList( &granuleList, granTempPtr + 1, &granListSize );
            if ( errStatus == EXIT_FAILURE )
            {
                FATAL_MSG("Failed to update the granule list.\n");
                goto cleanupFail;
            }

            /*This may be MOD02HKM or MOD03, so need to check */
            if(strstr(string,MODIScheck2) !=NULL)  /*Has MOD02HKM*/
            {

                /* Allocate memory */
                MODISargs[2] = malloc( strlen(string)+1 );
                memset(MODISargs[2],0,strlen(string)+1);
                strncpy( MODISargs[2], string, strlen(string));

                /* Get the MODIS 250m file */
                status = getNextLine( string, inputFile );
                if ( status == EXIT_FAILURE )
                {
                    FATAL_MSG("Failed to get MODIS line. Exiting program.\n");
                    goto cleanupFail;
                }

                
                granTempPtr = strrchr( string, '/' );
                if ( granTempPtr == NULL )
                {
                    FATAL_MSG("Failed to find the last occurance of slash character in the input line.\n");
                    goto cleanupFail;
                }
                errStatus = updateGranList( &granuleList, granTempPtr + 1, &granListSize );
                if ( errStatus == EXIT_FAILURE )
                {
                    FATAL_MSG("Failed to update the granule list.\n");
                    goto cleanupFail;
                }

                if ( strstr( string, MODIScheck3 ) == NULL )
                {
                    FATAL_MSG("Received an unexpected input line for MODIS.\n\tReceived string:\n\t%s\n\tExpected to receive string containing a substring of: %s\nExiting program.\n", string, MODIScheck3);
                    goto cleanupFail;
                }

                MODISargs[3] = malloc( strlen(string)+1 );
                memset(MODISargs[3],0,strlen(string)+1);
                strncpy( MODISargs[3], string, strlen(string) );

                /* Get the MODIS MOD03 file */
                status = getNextLine( string, inputFile );
                if ( status == EXIT_FAILURE )
                {
                    FATAL_MSG("Failed to get MODIS line. Exiting program.\n");
                    goto cleanupFail;
                }


                granTempPtr = strrchr( string, '/' );
                if ( granTempPtr == NULL )
                {
                    FATAL_MSG("Failed to find the last occurance of slash character in the input line.\n");
                    goto cleanupFail;
                }
                errStatus = updateGranList( &granuleList, granTempPtr + 1, &granListSize );
                if ( errStatus == EXIT_FAILURE )
                {
                    FATAL_MSG("Failed to update the granule list.\n");
                    goto cleanupFail;
                }

                if ( strstr( string, MODIScheck4 ) == NULL )
                {
                    FATAL_MSG("Received an unexpected input line for MODIS.\n\tReceived string:\n\t%s\n\tExpected to receive string containing a substring of: %s\nExiting program.\n", string, MODIScheck4);
                    goto cleanupFail;
                }

                MODISargs[4] = malloc( strlen( string )+1 );
                memset(MODISargs[4],0,strlen(string)+1);

                /* Remember the granule number */
                strncpy( MODISargs[4], string, strlen(string) );
                sprintf(modis_granule_suffix,"%d",modis_count);
                MODISargs[5] = malloc(strlen(granule)+strlen(modis_granule_suffix)+1);
                memset(MODISargs[5],0,strlen(granule)+strlen(modis_granule_suffix)+1);

                strncpy(MODISargs[5],granule,strlen(granule));
                strncat(MODISargs[5],modis_granule_suffix,strlen(modis_granule_suffix));

                status = MODIS( MODISargs,modis_count,unpack);
                if ( status == EXIT_FAILURE )
                {
                    FATAL_MSG("MODIS failed data transfer.\nExiting program.\n");
                    goto cleanupFail;
                }

            }

            else if(strstr(string,MODIScheck4)!=NULL)  /* No 500m and 250m */
            {

                /*Set 500m and 250m arguments to NULL */
                MODISargs[2] = NULL;
                MODISargs[3] = NULL;

                /* Allocate memory */
                MODISargs[4] = malloc( strlen( string )+1 );
                memset(MODISargs[4],0,strlen(string)+1);
                strncpy( MODISargs[4], string, strlen(string) );

                sprintf(modis_granule_suffix,"%d",modis_count);
                MODISargs[5] = malloc(strlen(granule)+strlen(modis_granule_suffix)+1);
                memset(MODISargs[5],0,strlen(granule)+strlen(modis_granule_suffix)+1);
                strncpy(MODISargs[5],granule,strlen(granule));
                strncat(MODISargs[5],modis_granule_suffix,strlen(modis_granule_suffix));

                status = MODIS( MODISargs,modis_count,unpack );
                if ( status == EXIT_FAILURE )
                {
                    FATAL_MSG("MODIS failed data transfer.\nExiting program.\n");
                    goto cleanupFail;
                }
            }

            else
            {
                FATAL_MSG("MODIS file order is not right. The current file should either be MOD03.. or MOD02HKM.. but it is %s\n", string);
                goto cleanupFail;
            }

            if(MODISargs[1]) free(MODISargs[1]);
            MODISargs[1] = NULL;
            if(MODISargs[2]) free(MODISargs[2]);
            MODISargs[2] = NULL;
            if(MODISargs[3]) free(MODISargs[3]);
            MODISargs[3] = NULL;
            if(MODISargs[4]) free(MODISargs[4]);
            MODISargs[4] = NULL;
            if(MODISargs[5]) free(MODISargs[5]);
            MODISargs[5] = NULL;

            /* get the next MODIS 1KM file */
            status = getNextLine( string, inputFile);
            if ( status == EXIT_FAILURE )
            {
                FATAL_MSG("Failed to get next line. Exiting program.\n");
                goto cleanupFail;
            }


            granTempPtr = strrchr( string, '/' );
            if ( granTempPtr == NULL )
            {
                FATAL_MSG("Failed to find the last occurance of slash character in the input line.\n");
                goto cleanupFail;
            }
            errStatus = updateGranList( &granuleList, granTempPtr + 1, &granListSize );
            if ( errStatus == EXIT_FAILURE )
            {
                FATAL_MSG("Failed to update the granule list.\n");
                goto cleanupFail;
            }

            modis_count++;

        }

        printf("MODIS done.\nTransferring ASTER...");
        fflush(stdout);
    }
    else
    {
        printf("No MODIS files found.\nTransferring ASTER...");
        fflush(stdout);
        status = getNextLine( string, inputFile);
        if ( status == EXIT_FAILURE )
        {
            FATAL_MSG("Failed to get next line. Exiting program.\n");
            goto cleanupFail;
        }


        granTempPtr = strrchr( string, '/' );
        if ( granTempPtr == NULL )
        {
            FATAL_MSG("Failed to find the last occurance of slash character in the input line.\n");
            goto cleanupFail;
        }
        errStatus = updateGranList( &granuleList, granTempPtr + 1, &granListSize );
        if ( errStatus == EXIT_FAILURE )
        {
            FATAL_MSG("Failed to update the granule list.\n");
            goto cleanupFail;
        }
    }
    /*********
     * ASTER *
     *********/

    ASTERargs[0] = argv[0];
    ASTERargs[3] = argv[1];

    /* Get the ASTER input files */
    /* MY 2016-12-20, Need to loop ASTER files since the number of granules may be different for each orbit */
    if ( strcmp(string, "AST N/A" ) )
    {
        while(aster_count != 0)
        {

            if(strstr( string, ASTERcheck ) != NULL )
            {

                /* Allocate memory for the argument */
                ASTERargs[1] = malloc( strlen(string )+1);
                memset(ASTERargs[1],0,strlen(string)+1);
                strncpy( ASTERargs[1], string, strlen(string) );

                /* Remember granule number */
                sprintf(aster_granule_suffix,"%d",aster_count);
                ASTERargs[2] = malloc(strlen(granule)+strlen(aster_granule_suffix)+1);
                memset(ASTERargs[2],0,strlen(granule)+strlen(aster_granule_suffix)+1);

                strncpy(ASTERargs[2],granule,strlen(granule));
                strncat(ASTERargs[2],aster_granule_suffix,strlen(aster_granule_suffix));

                /* EXECUTE ASTER DATA TRANSFER */
                status = ASTER( ASTERargs,aster_count,unpack);

                if ( status == EXIT_FAILURE )
                {
                    FATAL_MSG("ASTER failed data transfer.\nExiting program.\n");
                    goto cleanupFail;
                }

                aster_count++;
                free(ASTERargs[1]);
                ASTERargs[1] = NULL;
                free(ASTERargs[2]);
                ASTERargs[2] = NULL;

                status = getNextLine( string, inputFile );
                if ( status == EXIT_FAILURE )
                {
                    FATAL_MSG("Failed to get ASTER line. Exiting program.\n");
                    goto cleanupFail;
                }


                granTempPtr = strrchr( string, '/' );
                if ( granTempPtr == NULL )
                {
                    FATAL_MSG("Failed to find the last occurance of slash character in the input line.\n");
                    goto cleanupFail;
                }
                errStatus = updateGranList( &granuleList, granTempPtr + 1, &granListSize );
                if ( errStatus == EXIT_FAILURE )
                {
                    FATAL_MSG("Failed to update the granule list.\n");
                    goto cleanupFail;
                }
            }

            // Assume MISR is after ASTER
            else if(strstr(string,MISRcheck1)!=NULL)
                break;
            else
            {
                FATAL_MSG("either the ASTER file is wrong or the MISR_GRP file is not right after ASTER file.\n\tThe received line is %s.\n", string);
                goto cleanupFail;
            }
        }

        printf("ASTER done.\nTransferring MISR...");
        fflush(stdout);
    }
    else
    {
        printf("No ASTER files found.\nTransferring MISR...");
        fflush(stdout);
        status = getNextLine( string, inputFile);
        if ( status == EXIT_FAILURE )
        {
            FATAL_MSG("Failed to get next line. Exiting program.\n");
            goto cleanupFail;
        }


        granTempPtr = strrchr( string, '/' );
        if ( granTempPtr == NULL )
        {
            FATAL_MSG("Failed to find the last occurance of slash character in the input line.\n");
            goto cleanupFail;
        }
        errStatus = updateGranList( &granuleList, granTempPtr + 1, &granListSize );
        if ( errStatus == EXIT_FAILURE )
        {
            FATAL_MSG("Failed to update the granule list.\n");
            goto cleanupFail;
        }
    }

    /********
     * MISR *
     ********/
    MISRargs[0] = argv[0];

    if ( strcmp(string, "MIS N/A" ) )
    {
        for ( int i = 1; i < 10; i++ )
        {
            /* get the next MISR input file */
            if ( strstr( string, MISRcheck1 ) == NULL )
            {
                FATAL_MSG("Received an unexpected input line for MISR.\n\tReceived string:\n\t%s\n\tExpected to receive string containing a substring of: %s\nExiting program.\n", string, MISRcheck1);
                goto cleanupFail;
            }

            MISRargs[i] = malloc( strlen( string ) +1 );
            memset(MISRargs[i],0,strlen(string)+1);
            strncpy( MISRargs[i], string, strlen(string) );
            status = getNextLine( string, inputFile );
            if ( status == EXIT_FAILURE )
            {
                FATAL_MSG("Failed to get MISR line. Exiting program.\n");
                goto cleanupFail;
            }


            granTempPtr = strrchr( string, '/' );
            if ( granTempPtr == NULL )
            {
                FATAL_MSG("Failed to find the last occurance of slash character in the input line.\n");
                goto cleanupFail;
            }
            errStatus = updateGranList( &granuleList, granTempPtr + 1, &granListSize );
            if ( errStatus == EXIT_FAILURE )
            {
                FATAL_MSG("Failed to update the granule list.\n");
                goto cleanupFail;
            }
        }

        if ( strstr( string, MISRcheck2 ) == NULL )
        {
            FATAL_MSG("Received an unexpected input line for MISR.\n\tReceived string:\n\t%s\n\tExpected to receive string containing a substring of: %s\nExiting program.\n", string, MISRcheck2);
            goto cleanupFail;
        }

        MISRargs[10] = malloc ( strlen( string ) +1);
        memset(MISRargs[10],0,strlen(string)+1);
        strncpy( MISRargs[10], string, strlen(string) );
        status = getNextLine( string, inputFile );
        if ( status == EXIT_FAILURE )
        {
            FATAL_MSG("Failed to get MISR line. Exiting program.\n");
            goto cleanupFail;
        }

        if ( strstr( string, MISRcheck3 ) == NULL )
        {
            FATAL_MSG("Received an unexpected input line for MISR.\n\tReceived string:\n\t%s\n\tExpected to receive string containing a substring of: %s\nExiting program.\n", string, MISRcheck3);
            goto cleanupFail;
        }


        granTempPtr = strrchr( string, '/' );
        if ( granTempPtr == NULL )
        {
            FATAL_MSG("Failed to find the last occurance of slash character in the input line.\n");
            goto cleanupFail;
        }
        errStatus = updateGranList( &granuleList, granTempPtr + 1, &granListSize );
        if ( errStatus == EXIT_FAILURE )
        {
            FATAL_MSG("Failed to update the granule list.\n");
            goto cleanupFail;
        }

        MISRargs[11] = malloc ( strlen( string ) +1);
        memset(MISRargs[11],0,strlen(string)+1);
        strncpy( MISRargs[11], string, strlen(string) );

        status = getNextLine( string, inputFile );
        if ( status == EXIT_FAILURE )
        {
            FATAL_MSG("Failed to get MISR line. Exiting program.\n");
            goto cleanupFail;
        }

        if ( strstr( string, MISRcheck4 ) == NULL )
        {
            FATAL_MSG("Received an unexpected input line for MISR.\n\tReceived string:\n\t%s\n\tExpected to receive string containing a substring of: %s\nExiting program.\n", string, MISRcheck4);
            goto cleanupFail;
        }


        granTempPtr = strrchr( string, '/' );
        if ( granTempPtr == NULL )
        {
            FATAL_MSG("Failed to find the last occurance of slash character in the input line.\n");
            goto cleanupFail;
        }
        errStatus = updateGranList( &granuleList, granTempPtr + 1, &granListSize );
        if ( errStatus == EXIT_FAILURE )
        {
            FATAL_MSG("Failed to update the granule list.\n");
            goto cleanupFail;
        }

        MISRargs[12] = malloc ( strlen( string ) +1);
        memset(MISRargs[12],0,strlen(string)+1);
        strncpy( MISRargs[12], string, strlen(string) );

        // EXECUTE MISR DATA TRANSFER
        status = MISR( MISRargs,unpack);
        if ( status == EXIT_FAILURE )
        {
            FATAL_MSG("MISR failed data transfer.\nExiting program.\n");
            goto cleanupFail;
        }
        printf("MISR done.\n");
    }
    else
        printf("No MISR files found.\n");

    /* Attach the granuleList as an attribute to the root HDF5 object */
    errStatus = H5LTset_attribute_string( outputFile, "/", "Input Granules", granuleList);
    if ( errStatus < 0 )
    {
        FATAL_MSG("Failed to set Input Granules attribute in root group.\n");
        goto cleanupFail;
    }
    

    printf("Data transfer successful.\n");

    /* free all memory */

    if ( 0 )
    {
cleanupFail:
        fail = 1;
    }

    if ( outputFile ) H5Fclose(outputFile);
    if ( inputFile ) fclose(inputFile);
    if ( MOPITTargs[1] ) free(MOPITTargs[1]);
    if ( MODISargs[1] ) free(MODISargs[1]);
    if (  MODISargs[2] ) free( MODISargs[2]);
    if ( MODISargs[3] ) free ( MODISargs[3] );
    if ( MODISargs[4] ) free( MODISargs[4] );
    if ( MODISargs[5] ) free( MODISargs[5] );
    if ( ASTERargs[1] ) free ( ASTERargs[1] );
    if ( ASTERargs[2] ) free ( ASTERargs[2] );
    if ( TAI93toUTCoffset ) free(TAI93toUTCoffset);
    if ( test_orbit_ptr) free(test_orbit_ptr);
    if ( new_orbit_info_b) fclose(new_orbit_info_b);
    for ( int j = 1; j <= 12; j++ )
        if ( MISRargs[j] ) free (MISRargs[j]);
    if ( granuleList ) free(granuleList);

    if ( fail ) return -1;

    return 0;
}

int getNextLine ( char* string, FILE* const inputFile )
{

    do
    {
        if ( fgets( string, STR_LEN, inputFile ) == NULL )
        {
            if ( feof(inputFile) != 0 )
                return -1;
            FATAL_MSG("Unable to get next line.\n");
            return EXIT_FAILURE;
        }
    } while ( string[0] == '#' || string[0] == '\n' || string[0] == ' ' );

    /* remove the trailing newline or space character from the buffer if it exists */
    size_t len = strlen( string )-1;
    if ( string[len] == '\n' || string[len] == ' ')
        string[len] = '\0';

    return EXIT_SUCCESS;
}

/*
 *      updateGranList
 *
 * DESCRIPTION:
 *      This function handles creating a list of input granules as a string. It takes a running total of all currently visited
 *      granules, "granList", a new granule to add, "newGran", and appends newGran to granList. This handles all of the necessary
 *      memory operations for a variable length string. Adding a new granule to the list is an amortized constant operation.
 *
 * ARGUMETS:
 *      IN:
 *          const char newGran     -- A null terminated string containing the new granule. Should not have any newline characters.
 *                                    absence of the null terminator risks memory access violations!
 *      
 *      IN/OUT:
 *          char** granList        -- A pointer to a character array. This function updates the character array pointer as needed
 *                                    when allocating memory. The granList double pointer should point to a NULL value the first
 *                                    time this function is called because granList will be dereferenced once to contain the
 *                                    return value of malloc.
 *          size_t* curSize        -- The current size in bytes of the memory pointed to by *granList. Is updated when new memory
 *                                    is allocated. *curSize should be zero when first calling this function.
 *
 * EFFECTS:
 *      Allocates new memory as needed. Updates *granList and *curSize variables as needed.
 *
 *      IT IS THE DUTY OF THE CALLER to free the granList memory when finished!
 *
 * RETURN:
 *      EXIT_FAILURE.
 *      EXIT_SUCCESS.
 */

herr_t updateGranList( char** granList, const char* newGran, size_t * curSize )
{
    if ( granList == NULL || curSize == NULL || newGran == NULL )
    {
        FATAL_MSG("No arguments to this function can be NULL.\n\tgranList = %p\n\tcurSize = %p\n\tnewGran = %p\n", granList,
                   curSize, newGran);
        return EXIT_FAILURE;
    }
    size_t granLen = strlen( newGran );
    if ( granLen < 2 )
    {
        FATAL_MSG("The newGran argument contains no string!\n");
        return EXIT_FAILURE;
    }

    size_t granListSize = 0;

    if ( *granList != NULL )
        granListSize = strlen(*granList);

    if ( *granList == NULL )
    {
        if ( *curSize != 0 )
        {
            FATAL_MSG("A NULL pointer was passed for the string but the size was indicated to be non-zero.\n");
            return EXIT_FAILURE;
        }

        *granList = calloc( granLen+2, 1 );
        if ( *granList == NULL )
        {
            FATAL_MSG("Failed to allocate memory.\n");
            return EXIT_FAILURE;
        }

        *curSize = granLen+2;
    }

    else if ( granListSize + granLen + 2 > *curSize )
    {
        
        granListSize = strlen(*granList);
        // granLen+2 to account for newline and null terminator that will be added
        void* tempPtr = realloc( *granList, max((*curSize) * 2 , granListSize+granLen+2));
        if ( tempPtr == NULL )
        {
            FATAL_MSG("Failed to allocate memory.\n");
            return EXIT_FAILURE;
        }
        *granList = (char*) tempPtr;

        *curSize = max( (*curSize) * 2 , granListSize+granLen+2);
    }

    strncat( *granList, newGran, granLen );

    // Set the newline and null terminator on the ends of the granule list
    size_t newGranListSize = strlen(*granList);
    (*granList)[newGranListSize]   = '\n';
    (*granList)[newGranListSize+1] = '\0';

    return EXIT_SUCCESS;
}

#if 0
int validateInFiles( char* inFileList, OInfo_t orbitInfo )
{
    short fail = 0;
    const char MOPITTcheck[] = "MOP01";
    const char CEREScheck[] = "CER_SSF_Terra";
    const char MODIScheck1[] = "MOD021KM";
    const char MODIScheck2[] = "MOD02HKM";
    const char MODIScheck3[] = "MOD02QKM";
    const char MODIScheck4[] = "MOD03";
    const char ASTERcheck[] = "AST_L1T_";
    const char MISRcheck1[] = "MISR_AM1_GRP";
    const char MISRcheck2[] = "MISR_AM1_AGP";
    char* MODIStime = NULL;
    char tempString[STR_LEN];
    int i;
    herr_t status;

    memset(tempString, 0, STR_LEN);

    /* open the input file list */
    FILE* inputFile = fopen( inFileList, "r" );
    char string[STR_LEN];

    if ( inputFile == NULL )
    {
        FATAL_MSG("file \"%s\" does not exist\n", inFileList);
        return EXIT_FAILURE;
    }


    /* MOPITT and CERES don't need to be checked. The validity of those files will be
     * checked in the subsetting functions.
     */
    do
    {
        status = getNextLine(string, inputFile );
        if ( status == EXIT_FAILURE )
        {
            FATAL_MSG("Failed to get next line.\n");
            goto cleanupFail;
        }
    } while ( strstr(string, MODIScheck1) == NULL );



    /********************************************
     *                  MODIS                   *
     ********************************************/

    /* MODIS files have starting accuracy up to the minute */

    MODIStime = getTime(string, 2);
    if ( MODIStime == NULL )
    {
        FATAL_MSG("Failed to get MODIS time.\n");
        goto cleanupFail;
    }
    /* get MODIS year */
    for ( int i = 0; i < 4; i++ )
        tempString[i] = MODIStime[i];

    int MODISyear = atoi(tempString);
    if ( MODISyear == 0)
    {
        FATAL_MSG("Failed to get MODIS year.\n");
        goto cleanupFail;
    }

    memset(tempString, 0, STR_LEN);

    /* get MODIS proper day */
    for ( int i = 0; i < 3; i++ )
        tempString[i] = MODIStime[i+4];

    int MODISproperDay = atoi(tempString);
    if ( MODISproperDay == 0 )
    {
        FATAL_MSG("Failed to find proper MODIS day.\n");
        goto cleanupFail;
    }

    int MODISmonth;
    int MODISday;

    int leapYear = isLeapYear(MODISyear);
    if ( leapYear < 0 )
    {
        FATAL_MSG("Failed to determine if it is a leap year.\n");
        goto cleanupFail;
    }

    /* Using the MODIS proper day (which is a running total count of the days in the year)
     * find the day of the month.
     */

    if ( MODISproperDay >= 1 && MODISproperDay <= 31 )
    {
        MODISmonth = 1;
        MODISday = MODISproperDay;
    }
    else if ( MODISproperDay > 31 && MODISproperDay <= 59 + leapYear )
    {
        MODISmonth = 2;
        MODISday = MODISproperDay - 31;
    }
    else if ( MODISproperDay > 59 + leapYear && MODISproperDay <= 90 + leapYear )
    {
        MODISmonth = 3;
        MODISday = MODISproperDay - 59 + leapYear ;
    }
    else if ( MODISproperDay > 90  + leapYear&& MODISproperDay <= 120 + leapYear )
    {
        MODISmonth = 4;
        MODISday = MODISproperDay - 90 + leapYear;
    }
    else if ( MODISproperDay > 120 + leapYear && MODISproperDay <= 151 + leapYear )
    {
        MODISmonth = 5;
        MODISday = MODISproperDay - 120 + leapYear;
    }
    else if ( MODISproperDay > 151 + leapYear && MODISproperDay <= 181 + leapYear )
    {
        MODISmonth = 6;
        MODISday = MODISproperDay - 151 + leapYear;
    }
    else if ( MODISproperDay > 181 + leapYear && MODISproperDay <= 212 + leapYear )
    {
        MODISmonth = 7;
        MODISday = MODISproperDay - 181 + leapYear;
    }
    else if ( MODISproperDay > 212 + leapYear && MODISproperDay <= 243 + leapYear )
    {
        MODISmonth = 8;
        MODISday = MODISproperDay - 212 + leapYear;
    }
    else if ( MODISproperDay > 243 + leapYear && MODISproperDay <= 273 + leapYear )
    {
        MODISmonth = 9;
        MODISday = MODISproperDay - 243 + leapYear;
    }
    else if ( MODISproperDay > 273 + leapYear && MODISproperDay <= 304 + leapYear )
    {
        MODISmonth = 10;
        MODISday = MODISproperDay - 273 + leapYear;
    }
    else if ( MODISproperDay > 304 + leapYear && MODISproperDay <= 334 + leapYear )
    {
        MODISmonth = 11;
        MODISday = MODISproperDay - 304 + leapYear;
    }
    else if ( MODISproperDay > 334 + leapYear && MODISproperDay <= 365 + leapYear )
    {
        MODISmonth = 12;
        MODISday = MODISproperDay - 334 + leapYear;
    }
    else
    {
        FATAL_MSG("Failed to find the MODIS day.\n");
        goto cleanupFail;
    }

    /* Find the MODIS hour */

    memset(tempString, 0, STR_LEN);

    for ( int i = 0; i < 2; i++ )
        tempString[i] = MODIStime[i+8];

    int MODIShour = atoi(tempString);
    if ( MODIShour == 0 )
    {
        FATAL_MSG("Failed to find MODIS hour.\n");
        goto cleanupFail;
    }

    /* find MODIS minute */
    memset(tempString, 0, STR_LEN);


    for ( int i = 0; i < 2; i++ )
        tempString[i] = MODIStime[i+10];

    int MODISminute = atoi(tempString);
    if ( MODISminute == 0 )
    {
        FATAL_MSG("Failed to find MODIS minute.\n");
        goto cleanupFail;
    }

    /* Perform boundary checking */

    if ( ! (( MODISyear >= orbitInfo.start_year && MODISyear <= orbitInfo.end_year) &&
            ( MODISmonth >= orbitInfo.start_month && MODISmonth <= orbitInfo.end_month ) &&
            ( MODISday >= orbitInfo.start_day && MODISday <= orbitInfo.end_day ) &&
            ( MODIShour >= orbitInfo.start_hour && MODIShour <= orbitInfo.end_hour ) &&
            ( MODISminute >= orbitInfo.start_minute && MODISminute <= orbitInfo.end_minute ) ) )
    {
        FATAL_MSG("MODIS granule's starting time does not reside within orbit's start and end time.\n");
        goto cleanupFail;
    }

    /* next step is to make sure that the following MODIS files in the current MODIS group (either MOD03 or HKM AND QKM AND MOD03)
     * have the same date and time as the 1KM file. Note, each grouping of (1KM AND MOD03) OR (1KM AND HKM AND QKM AND MOD03)
     * is considered to be one group. The files MUST be ordered according to one of the two choices for a group.
     */


    if ( 0 )
    {
cleanupFail:
        fail = 1;
    }

    if (MODIStime) free(MODIStime);
    if ( fail )
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}
#endif
