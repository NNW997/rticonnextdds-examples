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
/* msg_publisher.cxx

   A publication of data of type msg

   This file is derived from code automatically generated by the rtiddsgen
   command:

   rtiddsgen -language C++ -example <arch> msg.idl

   Example publication of type msg automatically generated by
   'rtiddsgen'. To test them follow these steps:

   (1) Compile this file and the example subscription.

   (2) Start the subscription on the same domain used for RTI Data Distribution
       with the command
       objs/<arch>/msg_subscriber <domain_id> <sample_count>

   (3) Start the publication on the same domain used for RTI Data Distribution
       with the command
       objs/<arch>/msg_publisher <domain_id> <sample_count>

   (4) [Optional] Specify the list of discovery initial peers and
       multicast receive addresses via an environment variable or a file
       (in the current working directory) called NDDS_DISCOVERY_PEERS.

   You can run any number of publishers and subscribers programs, and can
   add and remove them dynamically from the domain.


   Example:

       To run the example application on domain <domain_id>:

       On Unix:

       objs/<arch>/msg_publisher <domain_id>
       objs/<arch>/msg_subscriber <domain_id>

       On Windows:

       objs\<arch>\msg_publisher <domain_id>
       objs\<arch>\msg_subscriber <domain_id>


modification history
------------ -------
* Add code to store keys of authorized participants

* Define listeners for builtin topics, which get called when
  we find a new participant or reader.

* Create disabled participant to ensure our listeners are installed
  before anything is processed

* Install listeners

* Uncommented the code that authorized a subscriber that belonged to an
authorized participant

* Changed ih to be an array of 6 ints instead of 4. An instance handle is the
size of 6 ints.
*/

#include "msg.h"
#include "msgSupport.h"
#include "ndds/ndds_cpp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Authorization string. */
const char *auth = "password";

/* The builtin subscriber sets participant_qos.user_data and
   so we set up listeners for the builtin
   DataReaders to access these fields.
*/

class BuiltinParticipantListener : public DDSDataReaderListener
{
public:
    virtual void on_data_available(DDSDataReader *reader);
};

/* This gets called when a participant has been discovered */
void BuiltinParticipantListener::on_data_available(DDSDataReader *reader)
{
    DDSParticipantBuiltinTopicDataDataReader *builtin_reader =
            (DDSParticipantBuiltinTopicDataDataReader *) reader;
    DDS_ParticipantBuiltinTopicDataSeq data_seq;
    DDS_SampleInfoSeq info_seq;
    DDS_ReturnCode_t retcode;

    const char *participant_data;

    /* We only process newly seen participants */
    retcode = builtin_reader->take(
            data_seq,
            info_seq,
            DDS_LENGTH_UNLIMITED,
            DDS_ANY_SAMPLE_STATE,
            DDS_NEW_VIEW_STATE,
            DDS_ANY_INSTANCE_STATE);

    /* This happens when we get announcements from participants we
     * already know about
     */
    if (retcode == DDS_RETCODE_NO_DATA)
        return;

    if (retcode != DDS_RETCODE_OK) {
        printf("***Error: failed to access data from the built-in reader\n");
        return;
    }

    for (int i = 0; i < data_seq.length(); ++i) {
        if (!info_seq[i].valid_data)
            continue;

        participant_data = "nil";
        bool is_auth = false;
        /* see if there is any participant_data */
        if (data_seq[i].user_data.value.length() != 0) {
            /* This sequence is guaranteed to be contiguous */
            participant_data = (char *) &data_seq[i].user_data.value[0];
            is_auth = (strcmp(participant_data, auth) == 0);
        }

        printf("Built-in Reader: found participant \n");
        printf("\tkey->'%08x %08x %08x'\n\tuser_data->'%s'\n",
               data_seq[i].key.value[0],
               data_seq[i].key.value[1],
               data_seq[i].key.value[2],
               participant_data);

        int ih[6];
        memcpy(ih,
               &info_seq[i].instance_handle,
               sizeof(info_seq[i].instance_handle));
        printf("instance_handle: %08x%08x %08x%08x %08x%08x \n",
               ih[0],
               ih[1],
               ih[2],
               ih[3],
               ih[4],
               ih[5]);

        if (!is_auth) {
            printf("Bad authorization, ignoring participant\n");
            DDSDomainParticipant *participant =
                    reader->get_subscriber()->get_participant();

            retcode = participant->ignore_participant(
                    info_seq[i].instance_handle);
            if (retcode != DDS_RETCODE_OK) {
                printf("error ignoring participant: %d\n", retcode);
                return;
            }
        }
    }

    builtin_reader->return_loan(data_seq, info_seq);
}

class BuiltinSubscriberListener : public DDSDataReaderListener
{
public:
    virtual void on_data_available(DDSDataReader *reader);
};

/* This gets called when a new subscriber has been discovered */
void BuiltinSubscriberListener::on_data_available(DDSDataReader *reader)
{
    DDSSubscriptionBuiltinTopicDataDataReader *builtin_reader =
            (DDSSubscriptionBuiltinTopicDataDataReader *) reader;
    DDS_SubscriptionBuiltinTopicDataSeq data_seq;
    DDS_SampleInfoSeq info_seq;
    DDS_ReturnCode_t retcode;

    /* We only process newly seen subscribers */
    retcode = builtin_reader->take(
            data_seq,
            info_seq,
            DDS_LENGTH_UNLIMITED,
            DDS_ANY_SAMPLE_STATE,
            DDS_NEW_VIEW_STATE,
            DDS_ANY_INSTANCE_STATE);

    if (retcode == DDS_RETCODE_NO_DATA)
        return;

    if (retcode != DDS_RETCODE_OK) {
        printf("***Error: failed to access data from the built-in reader\n");
        return;
    }

    for (int i = 0; i < data_seq.length(); ++i) {
        if (!info_seq[i].valid_data)
            continue;

        printf("Built-in Reader: found subscriber \n");
        printf("\tparticipant_key->'%08x %08x %08x'\n",
               data_seq[i].participant_key.value[0],
               data_seq[i].participant_key.value[1],
               data_seq[i].participant_key.value[2]);
        printf("\tkey->'%08x %08x %08x'\n",
               data_seq[i].key.value[0],
               data_seq[i].key.value[1],
               data_seq[i].key.value[2]);


        int ih[6];
        memcpy(ih,
               &info_seq[i].instance_handle,
               sizeof(info_seq[i].instance_handle));
        printf("instance_handle: %08x%08x %08x%08x %08x%08x \n",
               ih[0],
               ih[1],
               ih[2],
               ih[3],
               ih[4],
               ih[5]);
    }

    builtin_reader->return_loan(data_seq, info_seq);
}

/* End changes for Builtin_Topics */

/* Delete all entities */
static int publisher_shutdown(DDSDomainParticipant *participant)
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

    /* RTI Data Distribution Service provides finalize_instance() method for
       people who want to release memory used by the participant factory
       singleton. Uncomment the following block of code for clean destruction of
       the participant factory singleton. */
    /*
    retcode = DDSDomainParticipantFactory::finalize_instance();
    if (retcode != DDS_RETCODE_OK) {
        printf("finalize_instance error %d\n", retcode);
        status = -1;
    }
    */

    return status;
}

extern "C" int publisher_main(int domainId, int sample_count)
{
    DDSDomainParticipant *participant = NULL;
    DDSPublisher *publisher = NULL;
    DDSTopic *topic = NULL;
    DDSDataWriter *writer = NULL;
    msgDataWriter *msg_writer = NULL;
    msg *instance = NULL;
    DDS_ReturnCode_t retcode;
    DDS_InstanceHandle_t instance_handle = DDS_HANDLE_NIL;
    const char *type_name = NULL;
    int count = 0;
    DDS_Duration_t send_period = { 1, 0 };

    /* By default, the participant is enabled upon construction.
     * At that time our listeners for the builtin topics have not
     * been installed, so we disable the participant until we
     * set up the listeners. This is done by default in the
     * USER_QOS_PROFILES.xml
     * file. If you want to do it programmatically, just uncomment
     * the following code.
     */
    /*
    DDS_DomainParticipantFactoryQos factory_qos;
    retcode = DDSTheParticipantFactory->get_qos(factory_qos);
    if (retcode != DDS_RETCODE_OK) {
        printf("Cannot get factory Qos for domain participant\n");
        return -1;
    }

    factory_qos.entity_factory.autoenable_created_entities = DDS_BOOLEAN_FALSE;

    switch(DDSTheParticipantFactory->set_qos(factory_qos)) {
        case DDS_RETCODE_OK:
            break;
        case DDS_RETCODE_IMMUTABLE_POLICY: {
            printf("Cannot set factory Qos due to IMMUTABLE_POLICY ");
            printf("for domain participant\n");
            return -1;
            break;
        }
        case DDS_RETCODE_INCONSISTENT_POLICY: {
            printf("Cannot set factory Qos due to INCONSISTENT_POLICY for ");
            printf("domain participant\n");
            return -1;
            break;
        }
        default: {
            printf("Cannot set factory Qos for unknown reason for ");
            printf("domain participant\n");
            return -1;
            break;
        }
    }
    */

    DDS_DomainParticipantQos participant_qos;
    retcode = DDSTheParticipantFactory->get_default_participant_qos(
            participant_qos);
    if (retcode != DDS_RETCODE_OK) {
        printf("get_default_participant_qos error\n");
        return -1;
    }

    /* If you want to change the Participant's QoS programmatically rather
     * than using the XML file, you will need to uncomment the following line.
     */
    /*
    participant_qos.resource_limits.participant_user_data_max_length = 1024;
    */

    /* To customize participant QoS, use
       the configuration file USER_QOS_PROFILES.xml */
    participant = DDSTheParticipantFactory->create_participant(
            domainId,
            participant_qos,
            NULL /* listener */,
            DDS_STATUS_MASK_NONE);
    if (participant == NULL) {
        printf("create_participant error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* Start changes for Builtin_Topics */
    /* Installing listeners for the builtin topics requires several steps */

    /* First get the builtin subscriber */
    DDSSubscriber *builtin_subscriber = participant->get_builtin_subscriber();
    if (builtin_subscriber == NULL) {
        printf("***Error: failed to create builtin subscriber\n");
        return 0;
    }

    /* Then get builtin subscriber's datareader for participants
       The type name is a bit hairy, but can be read right to left:
       DDSParticipantBuiltinTopicDataDataReader is a DataReader for
       BuiltinTopicData concerning a discovered
       Participant
    */
    DDSParticipantBuiltinTopicDataDataReader *builtin_participant_datareader =
            (DDSParticipantBuiltinTopicDataDataReader *) builtin_subscriber
                    ->lookup_datareader(DDS_PARTICIPANT_TOPIC_NAME);
    if (builtin_participant_datareader == NULL) {
        printf("***Error: failed to create builtin participant data reader\n");
        return 0;
    }

    /* Install our listener */
    BuiltinParticipantListener *builtin_participant_listener =
            new BuiltinParticipantListener();
    builtin_participant_datareader->set_listener(
            builtin_participant_listener,
            DDS_DATA_AVAILABLE_STATUS);

    /* Get builtin subscriber's datareader for subscribers */
    DDSSubscriptionBuiltinTopicDataDataReader *builtin_subscription_datareader =
            (DDSSubscriptionBuiltinTopicDataDataReader *) builtin_subscriber
                    ->lookup_datareader(DDS_SUBSCRIPTION_TOPIC_NAME);
    if (builtin_subscription_datareader == NULL) {
        printf("***Error: failed to create builtin subscription data reader\n");
        return 0;
    }

    /* Install our listener */
    BuiltinSubscriberListener *builtin_subscriber_listener =
            new BuiltinSubscriberListener();
    builtin_subscription_datareader->set_listener(
            builtin_subscriber_listener,
            DDS_DATA_AVAILABLE_STATUS);

    /* Done!  All the listeners are installed, so we can enable the
     * participant now.
     */
    if (participant->enable() != DDS_RETCODE_OK) {
        printf("***Error: Failed to Enable Participant\n");
        return 0;
    }
    /* End changes for Builtin_Topics */

    /* To customize publisher QoS, use
       the configuration file USER_QOS_PROFILES.xml */
    publisher = participant->create_publisher(
            DDS_PUBLISHER_QOS_DEFAULT,
            NULL /* listener */,
            DDS_STATUS_MASK_NONE);
    if (publisher == NULL) {
        printf("create_publisher error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* Register the type before creating the topic */
    type_name = msgTypeSupport::get_type_name();
    retcode = msgTypeSupport::register_type(participant, type_name);
    if (retcode != DDS_RETCODE_OK) {
        printf("register_type error %d\n", retcode);
        publisher_shutdown(participant);
        return -1;
    }

    /* To customize topic QoS, use
       the configuration file USER_QOS_PROFILES.xml */
    topic = participant->create_topic(
            "Example msg",
            type_name,
            DDS_TOPIC_QOS_DEFAULT,
            NULL /* listener */,
            DDS_STATUS_MASK_NONE);
    if (topic == NULL) {
        printf("create_topic error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* To customize data writer QoS, use
       the configuration file USER_QOS_PROFILES.xml */
    writer = publisher->create_datawriter(
            topic,
            DDS_DATAWRITER_QOS_DEFAULT,
            NULL /* listener */,
            DDS_STATUS_MASK_NONE);

    if (writer == NULL) {
        printf("create_datawriter error\n");
        publisher_shutdown(participant);
        return -1;
    }
    msg_writer = msgDataWriter::narrow(writer);
    if (msg_writer == NULL) {
        printf("DataWriter narrow error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* Create data sample for writing */
    instance = msgTypeSupport::create_data();
    if (instance == NULL) {
        printf("msgTypeSupport::create_data error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* For data type that has key, if the same instance is going to be
       written multiple times, initialize the key here
       and register the keyed instance prior to writing */
    /*
    instance_handle = msg_writer->register_instance(*instance);
    */

    /* Main loop */
    for (count = 0; (sample_count == 0) || (count < sample_count); ++count) {
        NDDSUtility::sleep(send_period);

        printf("Writing msg, count %d\n", count);

        /* Modify the data to be sent here */
        instance->x = count;

        retcode = msg_writer->write(*instance, instance_handle);
        if (retcode != DDS_RETCODE_OK) {
            printf("write error %d\n", retcode);
        }
    }

    /*
    retcode = msg_writer->unregister_instance(
      *instance, instance_handle);
    if (retcode != DDS_RETCODE_OK) {
        printf("unregister instance error %d\n", retcode);
    }
    */

    /* Delete data sample */
    retcode = msgTypeSupport::delete_data(instance);
    if (retcode != DDS_RETCODE_OK) {
        printf("msgTypeSupport::delete_data error %d\n", retcode);
    }

    /* Delete all entities */
    return publisher_shutdown(participant);
}

int main(int argc, char *argv[])
{
    int domain_id = 0;
    int sample_count = 0; /* infinite loop */

    if (argc >= 2) {
        domain_id = atoi(argv[1]);
    }
    if (argc >= 3) {
        sample_count = atoi(argv[2]);
    }

    /* Uncomment this to turn on additional logging
    NDDSConfigLogger::get_instance()->
        set_verbosity_by_category(NDDS_CONFIG_LOG_CATEGORY_API,
                                  NDDS_CONFIG_LOG_VERBOSITY_STATUS_ALL);
    */

    return publisher_main(domain_id, sample_count);
}
