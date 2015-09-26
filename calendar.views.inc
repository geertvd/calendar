<?php

/**
 * @file
 * Provides views data for the calendar module.
 */

use Drupal\field\FieldStorageConfigInterface;

/**
 * Implements hook_views_data_alter().
 */
function calendar_views_data_alter(array &$data) {
  foreach ($data as $table_name => $table_data) {
    foreach ($table_data as $property_name => $properties) {
      if (isset($properties['argument']) && $properties['argument']['id'] === 'date') {
        $data[$table_name][$property_name . '_calendar'] = [
          'title' => t('Calendar contextual filter for @name', ['@name' => $property_name,]),
          'help' => t('A calendar contextual filter that handles granularity and range options.'),
          'argument' => [
            'field' => $property_name . '_calendar',
            'id' => 'calendar_datetime',
          ],
        ];
      }
    }
  }
}

/**
 * Implements hook_field_views_data_alter().
 */
function calendar_field_views_data_alter(array &$data, FieldStorageConfigInterface $field_storage) {
  if ($field_storage->getType() === 'datetime') {
    foreach ($data as $table_name => $table_data) {
      $data[$table_name][$field_storage->getName() . '_value_calendar'] = [
        'title' => t('Calendar contextual filter for @name', ['@name' => $field_storage->getName()]),
        'help' => t('A calendar contextual filter that handles granularity and range options.'),
        'argument' => [
          'field' => $field_storage->getName() . '_value',
          'id' => 'calendar_datetime',
        ],
        'group' => $data[$table_name][$field_storage->getName() . '_value']['group'],
      ];
    }
  }
}
