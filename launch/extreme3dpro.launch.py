import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    config = os.path.join(
        get_package_share_directory('extreme3dpro_driver'),
        'config','extreme3dpro.yaml'
    )

    return LaunchDescription([
        Node(
            package='extreme3dpro_driver',
            executable='extreme3dpro_node',
            name='extreme3dpro_driver',
            output='screen',
            parameters=[config],
            remappings=[
                ('joy',     '/joy'),
                ('cmd_vel', '/cmd_vel'),
                ('deadman', '/deadman'),
            ],
        ),
    ])
