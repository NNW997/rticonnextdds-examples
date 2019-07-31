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
/* ordered_subscriber.cxx

 A subscription example

 This file is derived from code automatically generated by the rtiddsgen
 command:

 rtiddsgen -language C++ -example <arch> ordered.idl

 Example subscription of type ordered automatically generated by
 'rtiddsgen'. To test them follow these steps:

 (1) Compile this file and the example publication.

 (2) Start the subscription with the command
 objs/<arch>/ordered_subscriber <domain_id> <sample_count>

 (3) Start the publication with the command
 objs/<arch>/ordered_publisher <domain_id> <sample_count>

 (4) [Optional] Specify the list of discovery initial peers and
 multicast receive addresses via an environment variable or a file
 (in the current working directory) called NDDS_DISCOVERY_PEERS.

 You can run any number of publishers and subscribers programs, and can
 add and remove them dynamically from the domain.


 Example:

 To run the example application on domain <domain_id>:

 On Unix:

 objs/<arch>/ordered_publisher <domain_id>
 objs/<arch>/ordered_subscriber <domain_id>

 On Windows:

 objs\<arch>\ordered_publisher <domain_id>
 objs\<arch>\ordered_subscriber <domain_id>


 modification history
 ------------ -------
 */

#include <stdio.h>
#include <stdlib.h>
#ifdef RTI_VX653
    #include <vThreadsData.h>
#endif
#include "ndds/ndds_cpp.h"
#include "ordered.h"
#include "orderedSupport.h"

#define MAX_SUBSCRIBERS 2
/* Start changes for Ordered_Presentation */

/* No listener is needed; we poll readers in this function */
void poll_data(orderedDataReader *ordered_reader[], int numreaders)
{
    DDS_ReturnCode_t retcode = DDS_RETCODE_OK;
    for (int r = 0; r < numreaders; ++r) {
        DDS_SampleInfoSeq info_seq;
        orderedSeq data_seq;
        retcode = ordered_reader[r]->take(
                data_seq,
                info_seq,
                DDS_LENGTH_UNLIMITED,
                DDS_ANY_SAMPLE_STATE,
                DDS_ANY_VIEW_STATE,
                DDS_ANY_INSTANCE_STATE);
        if (retcode == DDS_RETCODE_NO_DATA) {
            /* Not an error */
            continue;
        } else if (retcode != DDS_RETCODE_OK) {
            /* Is an error */
            printf("take error: %d\n", retcode);
            return;
        }

        for (int i = 0; i < data_seq.length(); ++i) {
            if (!info_seq[i].valid_data)
                continue;
            /* Make things a bit easier to read. */
            int ident = r;
            while (ident--) {
                printf("\t");
            }
            printf("Reader %d: Instance%d->value = %d\n",
                   r,
                   data_seq[i].id,
                   data_seq[i].value);
        }

        retcode = ordered_reader[r]->return_loan(data_seq, info_seq);
        if (retcode != DDS_RETCODE_OK) {
            printf("return loan error %d\n", retcode);
        }
    }
}

/* Delete all entities */
static int subscriber_shutdown(DDSDomainParticipant *participant)
{
    DDS_ReturnCode_t retcode;
    int status = 0;

    if (participant != NULL) {
        retcode = participant->delete_contained_entities();
        if (retcode != DDS_RETCODE_OK) {
            printf("delete_contained_entities error %d\n", retcode);
            status = -1;
        }

        retcode = DDSTheParticipantFactory->delete_participant(participant);
        if (retcode != DDS_RETCODE_OK) {
            printf("delete_participant error %d\n", retcode);
            status = -1;
        }
    }

    /* RTI Connext provides the finalize_instance() method on
     domain participant factory for people who want to release memory used
     by the participant factory. Uncomment the following block of code for
     clean destruction of the singleton. */
    /*
     retcode = DDSDomainParticipantFactory::finalize_instance();
     if (retcode != DDS_RETCODE_OK) {
     printf("finalize_instance error %d\n", retcode);
     status = -1;
     }
     */
    return status;
}

extern "C" int subscriber_main(int domainId, int sample_count)
{
    DDSDomainParticipant *participant = NULL;
    DDSSubscriber *subscriber[2] = { 0 };
    DDSTopic *topic = NULL;
    DDSDataReader *reader[2] = { 0 };
    DDS_ReturnCode_t retcode;
    const char *type_name = NULL;
    int count = 0;
    DDS_Duration_t receive_period = { 4, 0 };
    int status = 0;
    int i = 0;
    char *profile_name[] = { (char *) "ordered_Profile_subscriber_instance",
                             (char *) "ordered_Profile_subscriber_topic" };

    /* To customize the participant QoS, use
     the configuration file USER_QOS_PROFILES.xml */
    participant = DDSTheParticipantFactory->create_participant(
            domainId,
            DDS_PARTICIPANT_QOS_DEFAULT,
            NULL /* listener */,
            DDS_STATUS_MASK_NONE);
    if (participant == NULL) {
        printf("create_participant error\n");
        subscriber_shutdown(participant);
        return -1;
    }

    /* Register the type before creating the topic */
    type_name = orderedTypeSupport::get_type_name();
    retcode = orderedTypeSupport::register_type(participant, type_name);
    if (retcode != DDS_RETCODE_OK) {
        printf("register_type error %d\n", retcode);
        subscriber_shutdown(participant);
        return -1;
    }

    /* To customize the topic QoS, use
     the configuration file USER_QOS_PROFILES.xml */
    topic = participant->create_topic(
            "Example ordered",
            type_name,
            DDS_TOPIC_QOS_DEFAULT,
            NULL /* listener */,
            DDS_STATUS_MASK_NONE);
    if (topic == NULL) {
        printf("create_topic error\n");
        subscriber_shutdown(participant);
        return -1;
    }

    /* Start changes for ordered_presentation */

    /* Create two subscribers to illustrate different presentation QoS
     * This is a publisher/subscriber level QoS, so we have to
     * do it here instead of just making two datareaders
     */
    orderedDataReader *ordered_reader[2] = { 0 };

    /* Subscriber[0], reader[0] and ordered_reader[0] is getting
     * the profile "ordered_Profile_subscriber_instance"
     */
    /* Subscriber[1], reader[1] and ordered_reader[1] is getting
     * the profile "ordered_Profile_subscriber_topic"
     */
    for (i = 0; i < MAX_SUBSCRIBERS; ++i) {
        printf("Subscriber %d using %s\n", i, profile_name[i]);

        subscriber[i] = participant->create_subscriber_with_profile(
                "ordered_Library",
                profile_name[i],
                NULL,
                DDS_STATUS_MASK_NONE);
        if (subscriber[i] == NULL) {
            printf("create_subscriber%d error\n", i);
            subscriber_shutdown(participant);
            return -1;
        }

        reader[i] = subscriber[i]->create_datareader_with_profile(
                topic,
                "ordered_Library",
                profile_name[i],
                NULL,
                DDS_STATUS_MASK_ALL);
        if (reader[i] == NULL) {
            printf("create_datareader%d error\n", i);
            subscriber_shutdown(participant);
            return -1;
        }

        ordered_reader[i] = orderedDataReader::narrow(reader[i]);
        if (ordered_reader[i] == NULL) {
            printf("DataReader%d narrow error\n", i);
            return -1;
        }
    }

    /* If you want to change the Publisher's QoS programmatically rather than
     * using the XML file, you will need to add the following lines to your
     * code and comment out the above 'for' loop.
     */
    /*    for (int i = 0; i < MAX_SUBSCRIBERS; ++i) {
     */        /* Get default subscriber QoS to customize */
    /*        DDS_SubscriberQos subscriber_qos;
            retcode = participant->get_default_subscriber_qos(subscriber_qos);
            if (retcode != DDS_RETCODE_OK) {
                printf("get_default_subscriber_qos error\n");
                return -1;
            }

    */        /* Set this for both subscribers */
    /*        subscriber_qos.presentation.ordered_access = DDS_BOOLEAN_TRUE;

            if (i == 0) {
                printf("Subscriber 0 using Instance access scope\n");
                subscriber_qos.presentation.access_scope =
                        DDS_INSTANCE_PRESENTATION_QOS;
            } else {
                printf("Subscriber 1 using Topic access scope\n");
                subscriber_qos.presentation.access_scope =
                        DDS_TOPIC_PRESENTATION_QOS;
            }

    */        /* To create subscriber with default QoS, use
           * DDS_SUBSCRIBER_QOS_DEFAULT instead of subscriber_qos */
    /*        subscriber[i] = participant->create_subscriber(subscriber_qos,
       NULL, DDS_STATUS_MASK_NONE); if (subscriber[i] == NULL) {
                printf("create_subscriber error\n");
                subscriber_shutdown(participant);
                return -1;
            }

    */        /* No listener needed, but we do need to increase history depth */

    /* Get default datareader QoS to customize */
    /*        DDS_DataReaderQos datareader_qos;
            retcode = subscriber[i]->get_default_datareader_qos(datareader_qos);
            if (retcode != DDS_RETCODE_OK) {
                printf("get_default_datareader_qos error\n");
                return -1;
            }

            datareader_qos.history.depth = 10;

    */        /* To create datareader with default QoS,
           * use DDS_DATAREADER_QOS_DEFAULT instead of datareader_qos */
    /*        reader[i] = subscriber[i]->create_datareader(topic,
       datareader_qos, NULL, DDS_STATUS_MASK_ALL); if (reader[i] == NULL) {
                printf("create_datareader error\n");
                subscriber_shutdown(participant);
                return -1;
            }

            ordered_reader[i] = orderedDataReader::narrow(reader[i]);
            if (ordered_reader[i] == NULL) {
                printf("DataReader narrow error\n");
                return -1;
            }
        }

    */
    /* Poll for data every 4 seconds */

    /* Main loop */
    for (count = 0; (sample_count == 0) || (count < sample_count); ++count) {
        printf("ordered subscriber sleeping for %d sec...\n",
               receive_period.sec);
        NDDSUtility::sleep(receive_period);

        poll_data(ordered_reader, MAX_SUBSCRIBERS);
    }

    /* Delete all entities */
    status = subscriber_shutdown(participant);

    return status;
}

#if defined(RTI_WINCE)
int wmain(int argc, wchar_t **argv)
{
    int domainId = 0;
    int sample_count = 0; /* infinite loop */

    if (argc >= 2) {
        domainId = _wtoi(argv[1]);
    }
    if (argc >= 3) {
        sample_count = _wtoi(argv[2]);
    }

    /* Uncomment this to turn on additional logging
     NDDSConfigLogger::get_instance()->
     set_verbosity_by_category(NDDS_CONFIG_LOG_CATEGORY_API,
     NDDS_CONFIG_LOG_VERBOSITY_STATUS_ALL);
     */

    return subscriber_main(domainId, sample_count);
}

#elif !(defined(RTI_VXWORKS) && !defined(__RTP__)) && !defined(RTI_PSOS)
int main(int argc, char *argv[])
{
    int domainId = 0;
    int sample_count = 0; /* infinite loop */

    if (argc >= 2) {
        domainId = atoi(argv[1]);
    }
    if (argc >= 3) {
        sample_count = atoi(argv[2]);
    }

    /* Uncomment this to turn on additional logging
     NDDSConfigLogger::get_instance()->
     set_verbosity_by_category(NDDS_CONFIG_LOG_CATEGORY_API,
     NDDS_CONFIG_LOG_VERBOSITY_STATUS_ALL);
     */

    return subscriber_main(domainId, sample_count);
}
#endif

#ifdef RTI_VX653
const unsigned char *__ctype = *(__ctypePtrGet());

extern "C" void usrAppInit()
{
    #ifdef USER_APPL_INIT
    USER_APPL_INIT; /* for backwards compatibility */
    #endif

    /* add application specific code here */
    taskSpawn(
            "sub",
            RTI_OSAPI_THREAD_PRIORITY_NORMAL,
            0x8,
            0x150000,
            (FUNCPTR) subscriber_main,
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
