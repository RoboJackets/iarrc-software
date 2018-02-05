#include <rr_rviz_plugins/PosePanel.h>
#include <QVBoxLayout>
#include <pluginlib/class_list_macros.h>

/*
 * Just as in the header, everything needs to happen in the rr_rviz_plugins namespace.
 */
namespace rr_rviz_plugins {

PosePanel::PosePanel(QWidget *parent)
  : rviz::Panel(parent) // Base class constructor
{
    // Panels are allowed to interact with NodeHandles directly just like ROS nodes.
    ros::NodeHandle handle;

    // Initialize a label for displaying some data
    QLabel *label = new QLabel("0 m/s");
    QLabel *angleLabel = new QLabel("0 rad");

    /* Initialize our subscriber to listen to the /speed topic.
    * Note the use of boost::bind, which allows us to give the callback a pointer to our UI label.
    */
    speed_subscriber = handle.subscribe<rr_platform::speed>("/speed", 1, boost::bind(&PosePanel::speed_callback, this, _1, label));
    steer_subscriber = handle.subscribe<rr_platform::steering>("/steering", 1, boost::bind(&PosePanel::steering_callback, this, _1, angleLabel));

    /* Use QT layouts to add widgets to the panel.
    * Here, we're using a VBox layout, which allows us to stack all of our widgets vertically.
    */
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addWidget(angleLabel);
    setLayout(layout);
}

void PosePanel::speed_callback(const rr_platform::speedConstPtr &msg, QLabel *label) {
    // Create the new contents of the label based on the speed message.
    auto text = std::to_string(msg->speed) + " m/s";
    // Set the contents of the label.
    label->setText(text.c_str());
}

void PosePanel::steering_callback(const rr_platform::steeringConstPtr &msg, QLabel *label) {
    auto text = std::to_string(msg->angle) + " rad";
    label->setText(text.c_str());
}
//void ExamplePanel::paintEvent(QPaintEvent *event)  {
//
//}

}

/*
 * IMPORTANT! This macro must be filled out correctly for your panel class.
 */
PLUGINLIB_EXPORT_CLASS( rr_rviz_plugins::PosePanel, rviz::Panel)
