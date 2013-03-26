<?php

/**
 * @file
 * Contains the controller classes for Party entities.
 */

/**
 * The API controller class for the Party entity.
 */
class PartyController extends EntityAPIController {

  /**
   * Implements EntityAPIControllerInterface.
   *
   * Add in our additional logic for party labels. This is in here rather than
   * PartyController::save() so that we can get the label set as early as
   * possible. The tricky case is when a plugin requires the party to be saved
   * or we fall back to pid which also requires the party to be saved. For
   * those cases, we need to update the label immediately after we write the
   * record but before we invoke modules' implementation of
   * hook_entity_insert(). However, in the more common use case, we ideally
   * want the label to be set before hook_entity_presave() is invoked.
   */
  public function invoke($hook, $party) {
    // In presave, trigger our label and primary fields update without a save.
    if ($hook == 'presave') {
      $this->setLabel($party, FALSE);
      $this->setPrimaryFields($party, FALSE);
    }

    // In insert, if our label is empty, set our fallback and save.
    // @TODO: due to the use of the PID party label plugin as a stopper, we
    // we need to compare it to a stopper response from the plugin. Once we
    // rework the label plugins we can get rid of this uglyness and check
    // for an empty.
    if ($hook == 'insert' && $party->label == t('Party')) {
      $party->label = t('Party @pid', array('@pid' => $party->pid));
      db_query('UPDATE {party} SET label = :label WHERE pid = :pid', array(
        ':label' => $party->label,
        ':pid' => $party->pid,
      ));
    }

    // Trigger the rest of the invoke action.
    parent::invoke($hook, $party);
  }

  /**
   * Set the label on a Party object using the first label plugin
   * (when ordered by weight) that returns a non-empty value.
   *
   * This method optionally saves the label to the database as well as
   * generating it, this allows the function to be called independantly of
   * party save. Also, as most Party label plugins require the party to be
   * saved to function correctly, this avoids having to go through the
   * Party::save() method multiple times.
   *
   * @param $party
   *  A party object.
   * @param bool $store
   *  Whether we should store the new label.
   */
  public function setLabel($party, $store = TRUE) {
    module_load_include('inc', 'party', 'party.admin');

    // Get the label plugins.
    $label_plugins = party_settings_get_party_label_plugins();

    // Try each plugin, lowest weight first, until a plugin is
    // found that returns a non-empty value.
    foreach ($label_plugins as $path => $label_plugin) {
      $label_callback = $label_plugin['label callback'];
      $label = $label_callback($party);
      if (!empty($label)) {
        $party->label = $label;
        break;
      }
    }

    // No label plugins returned a label, use the party id if possible.
    if (empty($label) && isset($party->id)) {
      $party->label = t('Party @pid', array('@pid' => $party->id));
    }

    if ($store && isset($party->pid)) {
      // Save the label to the database.
      db_query('UPDATE {party} SET label = :label WHERE pid = :pid', array(
        ':label' => $party->label,
        ':pid' => $party->pid,
      ));

      $this->invoke('update', $party);
    }
  }

  /**
   * Set the primary fields for the party.
   *
   * @param $party
   *  A party object.
   */
  public function setPrimaryFields($party, $store = TRUE) {
    $primary_fields = variable_get('party_primary_fields', array());

    // Update the email.
    if (!empty($primary_fields['email'])) {
      // Extract our info from the setting.
      list($data_set_name, $field_name, $column) = explode(':', $primary_fields['email']);
      $party->email = party_get_data_set_value($party, $data_set_name, $column, $field_name);
    }

    if (empty($party->email) && !empty($primary_fields['email2'])) {
      // Extract our info from the setting.
      list($data_set_name, $field_name, $column) = explode(':', $primary_fields['email2']);
      $party->email = party_get_data_set_value($party, $data_set_name, $column, $field_name);
    }

    if ($store && isset($party->pid)) {
      // Save the primary feilds to the database.
      db_query('UPDATE {party} SET email = :email WHERE pid = :pid', array(
        ':email' => $party->email,
        ':pid' => $party->pid,
      ));
    }
  }

  /**
   * Overriding the buildContent function to add entity specific fields.
   */
  public function buildContent($entity, $view_mode = 'full', $langcode = NULL, $content = array()) {
    $content = parent::buildContent($entity, $view_mode, $langcode, $content);
    return $content;
  }
}

/**
 * The class used for party entities.
 */
class Party extends Entity {

  /**
   * The internal, numeric party-id.
   *
   * @var integer
   */
  public $pid;

  /**
   * The human-readable label for the party.
   *
   * @var string
   *
   * @see PartyController::setLabel()
   */
  public $label;

  /**
   * Boolean representing whether this party has been merged into another
   * party.
   *
   * @var integer
   */
  public $merged;

  /**
   * If $merged is TRUE, this is the pid of the party that this party has been
   * merged into.
   *
   * @var integer
   */
  public $merged_party;

  /**
   * The primary email for this party.
   *
   * @var string
   */
  public $email;

  /**
   * A cache of data set controllers.
   *
   * @var array
   */
  public $data_set_controllers = array();

  public function __construct($values = array()) {
    parent::__construct($values, 'party');
  }

  /**
   * Return the default label for this party object.
   *
   * @return string
   *   Human readable label for the party object.
   */
  protected function defaultLabel() {
    return $this->label;
  }

  /**
   * Return the default path for this party.
   *
   * @return array
   *   An array with the following key:
   *   - path - the drupal path for the party.
   */
  protected function defaultUri() {
    return array('path' => 'party/' . $this->pid);
  }
}
