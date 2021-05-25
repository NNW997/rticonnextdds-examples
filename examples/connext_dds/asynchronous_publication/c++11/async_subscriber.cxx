/*******************************************************************************
 (c) 2005-2015 Copyright, Real-Time Innovations, Inc.  All rights reserved.
 RTI grants Licensee a license to use, modify, compile, and create derivative
 works of the Software.  Licensee has the right to distribute object form only
 for use with RTI products.  The Software is provided "as is", with no warranty
 of any type, including any warranty for fitness for any purpose. RTI is under
 no obligation to maintain or support the Software.  RTI shall not be liable for
 any incidental or consequential damages arising out of the use or inability to
 use the software.
 ******************************************************************************/

#include <algorithm>
#include <iostream>

#include <dds/sub/ddssub.hpp>
#include <dds/core/ddscore.hpp>
#include <rti/config/Logger.hpp>  // for logging

#include "async.hpp"
#include "application.hpp"  // for command line parsing and ctrl-c

// For timekeeping
clock_t InitTime;

int process_data(dds::sub::DataReader<async> reader)
{
    dds::sub::LoanedSamples<async> samples = reader.take();
    for (const auto &sample : samples) {
        // Print the time we get each sample.
        if (sample.info().valid()) {
            double elapsed_ticks = clock() - InitTime;
            double elapsed_secs = elapsed_ticks / CLOCKS_PER_SEC;
            std::cout << "@ t=" << elapsed_secs << "s"
                      << ", got x = " << sample.data().x() << std::endl;
        }
    }
}  // The LoanedSamples destructor returns the loan

void run_subscriber_application(
        unsigned int domain_id,
        unsigned int sample_count)
{
    // For timekeeping
    InitTime = clock();

    // To customize the paritcipant QoS, use the file USER_QOS_PROFILES.xml
    dds::domain::DomainParticipant participant(domain_id);

    // To customize the topic QoS, use the file USER_QOS_PROFILES.xml
    dds::topic::Topic<async> topic(participant, "Example async");

    // Retrieve the default DataReader QoS, from USER_QOS_PROFILES.xml
    dds::sub::qos::DataReaderQos reader_qos =
            dds::core::QosProvider::Default().datareader_qos();

    // If you want to change the DataWriter's QoS programmatically rather than
    // using the XML file, uncomment the following lines.
    // reader_qos << Reliability::Reliable();
    // reader_qos << History::KeepAll();

    // Create a DataReader with a QoS
    // Create a Subscriber and DataReader with default Qos
    dds::sub::Subscriber subscriber(participant);
    dds::sub::DataReader<async> reader(subscriber, topic, reader_qos);

    // WaitSet will be woken when the attached condition is triggered
    dds::core::cond::WaitSet waitset;

    // Create a ReadCondition for any data on this reader, and add to WaitSet
    unsigned int samples_read = 0;
    dds::sub::cond::ReadCondition read_condition(
            reader,
            dds::sub::status::DataState::new_data(),
            [reader, &samples_read]() {
                // If we wake up, process data
                samples_read += process_data(reader);
            });

    waitset += read_condition;

    // Main loop
    while (!application::shutdown_requested && samples_read < sample_count) {
        std::cout << "async subscriber sleeping up to 4 sec..." << std::endl;

        // Wait for data and report if it does not arrive in 1 second
        waitset.dispatch(dds::core::Duration(4));
    }
}

int main(int argc, char *argv[])
{
    using namespace application;

    // Parse arguments and handle control-C
    auto arguments = parse_arguments(argc, argv);
    if (arguments.parse_result == ParseReturn::exit) {
        return EXIT_SUCCESS;
    } else if (arguments.parse_result == ParseReturn::failure) {
        return EXIT_FAILURE;
    }
    setup_signal_handlers();

    // Sets Connext verbosity to help debugging
    rti::config::Logger::instance().verbosity(arguments.verbosity);

    try {
        run_subscriber_application(arguments.domain_id, arguments.sample_count);
    } catch (const std::exception &ex) {
        // This will catch DDS exceptions
        std::cerr << "Exception in run_subscriber_application(): " << ex.what()
                  << std::endl;
        return EXIT_FAILURE;
    }

    // Releases the memory used by the participant factory.  Optional at
    // application exit
    dds::domain::DomainParticipant::finalize_participant_factory();

    return EXIT_SUCCESS;
}
