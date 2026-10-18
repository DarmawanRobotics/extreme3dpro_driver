import os

import yaml
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def node(context):
    with open(LaunchConfiguration('config').perform(context)) as f:
        params = yaml.safe_load(f) or {}
    params['device'] = LaunchConfiguration('device').perform(context)
    return [Node(
        package='extreme3dpro_driver',
        executable='extreme3dpro_node',
        name='extreme3dpro_driver',
        output='screen',
        parameters=[params],
    )]


def generate_launch_description():
    share = get_package_share_directory('extreme3dpro_driver')
    return LaunchDescription([
        DeclareLaunchArgument('device', default_value='/dev/input/js0'),
        DeclareLaunchArgument('config',
                              default_value=os.path.join(share, 'config', 'extreme3dpro.yaml')),
        OpaqueFunction(function=node),
    ])
