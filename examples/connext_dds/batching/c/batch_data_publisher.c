/*******************************************************************************
 (c) 2005-2014 Copyright, Real-Time Innovations, Inc.  All rights reserved.
 RTI grants Licensee a license to use, modify, compile, and create derivative
 works of the Software.  Licensee has the right to distribute object form only
 for use with RTI products.  The Software is provided "as is", with no warranty
 of any type, including any warranty for fitness for any purpose. RTI is under
 no obligation to maintain or support the Software.  RTI shall not be liable for
 any incidental or consequential damages arising out of the use or inability to
 use the software.
 ******************************************************************************/
/* batch_data_publisher.c

   A publication of data of type batch_data

   This file is derived from code automatically generated by the rtiddsgen
   command:

   rtiddsgen -language C -example <arch> batch_data.idl

   Example publication of type batch_data automatically generated by
   'rtiddsgen'. To test them follow these steps:

   (1) Compile this file and the example subscription.

   (2) Start the subscription with the command
       objs/<arch>/batch_data_subscriber <domain_id> <sample_count>

   (3) Start the publication with the command
       objs/<arch>/batch_data_publisher <domain_id> <sample_count>

   (4) [Optional] Specify the list of discovery initial peers and
       multicast receive addresses via an environment variable or a file
       (in the current working directory) called NDDS_DISCOVERY_PEERS.

   You can run any number of publishers and subscribers programs, and can
   add and remove them dynamically from the domain.


   Example:

       To run the example application on domain <domain_id>:

       On Unix:

       objs/<arch>/batch_data_publisher <domain_id>
       objs/<arch>/batch_data_subscriber <domain_id>

       On Windows:

       objs\<arch>\batch_data_publisher <domain_id>
       objs\<arch>\batch_data_subscriber <domain_id>


modification history
------------ -------
*/

#include "batch_data.h"
#include "batch_dataSupport.h"
#include "ndds/ndds_c.h"
#include <stdio.h>
#include <stdlib.h>

/* Delete all entities */
static int publisher_shutdown(DDS_DomainParticipant *participant)
{
    DDS_ReturnCode_t retcode;
    int status = 0;

    if (participant != NULL) {
        retcode = DDS_DomainParticipant_delete_contained_entities(participant);
        if (retcode != DDS_RETCODE_OK) {
            printf("delete_contained_entities error %d\n", retcode);
            status = -1;
        }

        retcode = DDS_DomainParticipantFactory_delete_participant(
                DDS_TheParticipantFactory,
                participant);
        if (retcode != DDS_RETCODE_OK) {
            printf("delete_participant error %d\n", retcode);
            status = -1;
        }
    }

    /* RTI Connext provides finalize_instance() method on
       domain participant factory for people who want to release memory used
       by the participant factory. Uncomment the following block of code for
       clean destruction of the singleton. */
    /*
        retcode = DDS_DomainParticipantFactory_finalize_instance();
        if (retcode != DDS_RETCODE_OK) {
            printf("finalize_instance error %d\n", retcode);
            status = -1;
        }
    */

    return status;
}

static int publisher_main(int domainId, int sample_count, int turbo_mode_on)
{
    DDS_DomainParticipant *participant = NULL;
    DDS_Publisher *publisher = NULL;
    DDS_Topic *topic = NULL;
    DDS_DataWriter *writer = NULL;
    batch_dataDataWriter *batch_data_writer = NULL;
    batch_data *instance = NULL;
    DDS_ReturnCode_t retcode;
    DDS_InstanceHandle_t instance_handle = DDS_HANDLE_NIL;
    const char *type_name = NULL;
    int count = 0;
    struct DDS_Duration_t send_period = { 1, 0 };
    const char *profile_name = NULL;
    const char *library_name = DDS_String_dup("batching_Library");

    /* We pick the profile name if the turbo_mode is selected or not.
     * If Turbo_mode is not selected, the batching profile will be used.
     */
    if (turbo_mode_on) {
        profile_name = DDS_String_dup("turbo_mode_profile");
        /* In turbo_mode, we do not want to wait to send samples */
        send_period.sec = 0;
        printf("Turbo Mode enable\n");
    } else {
        profile_name = DDS_String_dup("batch_profile");
        printf("Manual batching enable\n");
    }

    /* To customize participant QoS, use
       the configuration file USER_QOS_PROFILES.xml */
    participant = DDS_DomainParticipantFactory_create_participant_with_profile(
            DDS_TheParticipantFactory,
            domainId,
            library_name,
            profile_name,
            NULL /* listener */,
            DDS_STATUS_MASK_NONE);
    if (participant == NULL) {
        printf("create_participant error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* To customize publisher QoS, use
       the configuration file USER_QOS_PROFILES.xml */
    publisher = DDS_DomainParticipant_create_publisher_with_profile(
            participant,
            library_name,
            profile_name,
            NULL /* listener */,
            DDS_STATUS_MASK_NONE);
    if (publisher == NULL) {
        printf("create_publisher error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* Register type before creating topic */
    type_name = batch_dataTypeSupport_get_type_name();
    retcode = batch_dataTypeSupport_register_type(participant, type_name);
    if (retcode != DDS_RETCODE_OK) {
        printf("register_type error %d\n", retcode);
        publisher_shutdown(participant);
        return -1;
    }

    /* To customize topic QoS, use
       the configuration file USER_QOS_PROFILES.xml */
    topic = DDS_DomainParticipant_create_topic(
            participant,
            "Example batch_data",
            type_name,
            &DDS_TOPIC_QOS_DEFAULT,
            NULL /* listener */,
            DDS_STATUS_MASK_NONE);
    if (topic == NULL) {
        printf("create_topic error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* To customize data writer QoS, use
       the configuration file USER_QOS_PROFILES.xml */
    writer = DDS_Publisher_create_datawriter_with_profile(
            publisher,
            topic,
            library_name,
            profile_name,
            NULL /* listener */,
            DDS_STATUS_MASK_NONE);
    if (writer == NULL) {
        printf("create_datawriter error\n");
        publisher_shutdown(participant);
        return -1;
    }
    batch_data_writer = batch_dataDataWriter_narrow(writer);
    if (batch_data_writer == NULL) {
        printf("DataWriter narrow error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* Create data sample for writing */

    instance = batch_dataTypeSupport_create_data_ex(DDS_BOOLEAN_TRUE);

    if (instance == NULL) {
        printf("batch_dataTypeSupport_create_data error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* For a data type that has a key, if the same instance is going to be
       written multiple times, initialize the key here
       and register the keyed instance prior to writing */
    /*
        instance_handle = batch_dataDataWriter_register_instance(
            batch_data_writer, instance);
    */

    /* Main loop */
    for (count = 0; (sample_count == 0) || (count < sample_count); ++count) {
        printf("Writing batch_data, count %d\n", count);

        /* Modify the data to be written here */
        instance->x = count;

        /* Write data */
        retcode = batch_dataDataWriter_write(
                batch_data_writer,
                instance,
                &instance_handle);
        if (retcode != DDS_RETCODE_OK) {
            printf("write error %d\n", retcode);
        }

        NDDS_Utility_sleep(&send_period);
    }

    /*
        retcode = batch_dataDataWriter_unregister_instance(
            batch_data_writer, instance, &instance_handle);
        if (retcode != DDS_RETCODE_OK) {
            printf("unregister instance error %d\n", retcode);
        }
    */

    /* Delete data sample */
    retcode = batch_dataTypeSupport_delete_data_ex(instance, DDS_BOOLEAN_TRUE);
    if (retcode != DDS_RETCODE_OK) {
        printf("batch_dataTypeSupport_delete_data error %d\n", retcode);
    }

    /* Cleanup and delete delete all entities */
    return publisher_shutdown(participant);
}

#if defined(RTI_WINCE)
int wmain(int argc, wchar_t **argv)
{
    int domainId = 0;
    int sample_count = 0; /* infinite loop */
    int turbo_mode_on = 0;

    if (argc >= 2) {
        domainId = _wtoi(argv[1]);
    }
    if (argc >= 3) {
        turbo_mode_on = _wtoi(argv[2]);
    }
    if (argc >= 4) {
        sample_count = _wtoi(argv[3]);
    }

    /* Uncomment this to turn on additional logging
    NDDS_Config_Logger_set_verbosity_by_category(
        NDDS_Config_Logger_get_instance(),
        NDDS_CONFIG_LOG_CATEGORY_API,
        NDDS_CONFIG_LOG_VERBOSITY_STATUS_ALL);
    */

    return publisher_main(domainId, sample_count, turbo_mode_on);
}
#elif !(defined(RTI_VXWORKS) && !defined(__RTP__)) && !defined(RTI_PSOS)
int main(int argc, char *argv[])
{
    int domainId = 0;
    int sample_count = 0; /* infinite loop */
    int turbo_mode_on = 0;

    if (argc >= 2) {
        domainId = atoi(argv[1]);
    }
    if (argc >= 3) {
        turbo_mode_on = atoi(argv[2]);
    }
    if (argc >= 4) {
        sample_count = atoi(argv[3]);
    }

    /* Uncomment this to turn on additional logging
    NDDS_Config_Logger_set_verbosity_by_category(
        NDDS_Config_Logger_get_instance(),
        NDDS_CONFIG_LOG_CATEGORY_API,
        NDDS_CONFIG_LOG_VERBOSITY_STATUS_ALL);
    */

    return publisher_main(domainId, sample_count, turbo_mode_on);
}
#endif

#ifdef RTI_VX653
const unsigned char *__ctype = NULL;

void usrAppInit()
{
    #ifdef USER_APPL_INIT
    USER_APPL_INIT; /* for backwards compatibility */
    #endif

    /* add application specific code here */
    taskSpawn(
            "pub",
            RTI_OSAPI_THREAD_PRIORITY_NORMAL,
            0x8,
            0x150000,
            (FUNCPTR) publisher_main,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0);
}
#endif
