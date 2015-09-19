<?php
/**
 * @file
 * Contains \Drupal\calendar\CalendarEvent.
 */

namespace Drupal\calendar;

/**
 * Defines a calendar event object.
 */
class CalendarEvent {

  /**
   * @var int $entityId
   *   The id of the entity for this event.
   */
  protected $entityId;

  /**
   * @var string $entityType
   *   The type id of the entity for this event.
   */
  protected $entityTypeId;

  /**
   * @var \DateTime $start_date
   *   The start date of the event.
   */
  protected $start_date;

  /**
   * @var \DateTime $end_date
   *   The end date of the event.
   */
  protected $end_date;

  /**
   * @var string $title
   *   The title of the event.
   */
  protected $title;

  /**
   * @var string $url
   *   The public url for this event.
   */
  protected $url;

  /**
   * Getter for the entity id.
   *
   * @return int mixed
   *   The entity id.
   */
  public function getEntityId() {
    return $this->entityId;
  }

  /**
   * Setter for the entity id.
   *
   * @param int $entityId
   *   The entity id.
   */
  public function setEntityId($entityId) {
    $this->entityId = $entityId;
  }

  /**
   * Getter for the entity type id.
   *
   * @return string
   *   The entity type id.
   */
  public function getEntityTypeId() {
    return $this->entityTypeId;
  }

  /**
   * Setter for the entity type d.
   *
   * @param string $entityTypeId
   *   The entity type id.
   */
  public function setEntityTypeId($entityTypeId) {
    $this->entityTypeId = $entityTypeId;
  }

  /**
   * Getter for the start date.
   *
   * @return \DateTime
   *   The start date.
   */
  public function getStartDate() {
    return $this->start_date;
  }

  /**
   * Setter for the start date.
   *
   * @param \DateTime $start_date
   *   The start date.
   */
  public function setStartDate($start_date) {
    $this->start_date = $start_date;
  }

  /**
   * Getter for the end date.
   *
   * @return \DateTime
   *   The end date.
   */
  public function getEndDate() {
    return $this->end_date;
  }

  /**
   * Setter for the end date.
   *
   * @param \DateTime $end_date
   *   The end date.
   */
  public function setEndDate($end_date) {
    $this->end_date = $end_date;
  }

  /**
   * The title getter.
   *
   * @return string
   *   The title of the event.
   */
  public function getTitle() {
    return $this->title;
  }

  /**
   * The title setter.
   *
   * @param string $title
   *   The title of the event.
   */
  public function setTitle($title) {
    $this->title = $title;
  }

  /**
   * Getter for the url.
   *
   * @return string
   *   The public url to this event.
   */
  public function getUrl() {
    return $this->url;
  }

  /**
   * Setter for the url.
   *
   * @param string $url
   *   The public url to this event.
   */
  public function setUrl($url) {
    $this->url = $url;
  }

}