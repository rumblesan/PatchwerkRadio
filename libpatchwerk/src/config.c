#include <stdlib.h>

#include <libconfig.h>

#include "config.h"

#include "bclib/dbg.h"
#include "logging.h"

int config_setting_lookup_bstring(const config_setting_t *setting,
                                  const char *path, bstring *value) {
  const char *stringval;
  int rv = config_setting_lookup_string(setting, path, &stringval);
  if (rv)
    *value = bfromcstr(stringval);
  return rv;
}

RadioInputCfg *read_config(char *config_path) {
  logger("Config", "config path %s", config_path);
  config_t config;
  config_t *cfg = &config;

  RadioInputCfg *radio_config = malloc(sizeof(RadioInputCfg));
  check_mem(radio_config);

  config_init(cfg);

  check(config_read_file(cfg, config_path),
        "Could not read config file - %s:%d - %s", config_error_file(cfg),
        config_error_line(cfg), config_error_text(cfg));

  config_setting_t *audiosetting = config_lookup(cfg, "audio");
  check(audiosetting != NULL &&
            config_setting_lookup_int(audiosetting, "channels",
                                      &(radio_config->audio.channels)) &&
            config_setting_lookup_int(audiosetting, "samplerate",
                                      &(radio_config->audio.samplerate)) &&
            config_setting_lookup_float(audiosetting, "fadetime",
                                        &(radio_config->audio.fadetime)),
        "Could not read audio settings");

  // Patch Chooser config
  config_setting_t *choosersetting = config_lookup(cfg, "patchchooser");
  check(choosersetting != NULL &&
            config_setting_lookup_bstring(choosersetting, "pattern",
                                          &(radio_config->chooser.pattern)),
        "Could not read patch chooser settings");

  // PureData config
  config_setting_t *pdsetting = config_lookup(cfg, "puredata");
  check(pdsetting != NULL &&
            config_setting_lookup_bstring(
                pdsetting, "patch_directory",
                &(radio_config->puredata.patch_directory)) &&
            config_setting_lookup_bstring(pdsetting, "patch_file",
                                          &(radio_config->puredata.patch_file)),
        "Could not read pure data settings");

  // Encoder config
  config_setting_t *encsetting = config_lookup(cfg, "encoder");
  check(encsetting != NULL &&
            config_setting_lookup_float(encsetting, "quality",
                                        &(radio_config->encoder.quality)),
        "Could not read encoder settings");

  // Broadcast config
  config_setting_t *broadcastsetting = config_lookup(cfg, "broadcast");
  check(
      broadcastsetting != NULL &&
          config_setting_lookup_bstring(broadcastsetting, "host",
                                        &(radio_config->broadcast.host)) &&
          config_setting_lookup_int(broadcastsetting, "port",
                                    &(radio_config->broadcast.port)) &&
          config_setting_lookup_bstring(broadcastsetting, "source",
                                        &(radio_config->broadcast.source)) &&
          config_setting_lookup_bstring(broadcastsetting, "password",
                                        &(radio_config->broadcast.password)) &&
          config_setting_lookup_bstring(broadcastsetting, "mount",
                                        &(radio_config->broadcast.mount)) &&
          config_setting_lookup_bstring(broadcastsetting, "name",
                                        &(radio_config->broadcast.name)) &&
          config_setting_lookup_bstring(
              broadcastsetting, "description",
              &(radio_config->broadcast.description)) &&
          config_setting_lookup_bstring(broadcastsetting, "genre",
                                        &(radio_config->broadcast.genre)) &&
          config_setting_lookup_bstring(broadcastsetting, "url",
                                        &(radio_config->broadcast.url)),
      "Could not read broadcast settings");

  // Encoder config
  config_setting_t *syssetting = config_lookup(cfg, "system");
  check(
      syssetting != NULL &&
          config_setting_lookup_int(syssetting, "thread_sleep",
                                    &(radio_config->system.thread_sleep)) &&
          config_setting_lookup_int(syssetting, "stats_interval",
                                    &(radio_config->system.stats_interval)) &&
          config_setting_lookup_int(syssetting, "max_push_messages",
                                    &(radio_config->system.max_push_messages)),
      "Could not read system settings");

  if (cfg != NULL)
    config_destroy(cfg);
  return radio_config;
error:
  if (cfg != NULL)
    config_destroy(cfg);
  if (radio_config != NULL)
    free(radio_config);
  return NULL;
};

void destroy_config(RadioInputCfg *cfg) {
  check(cfg != NULL, "Invalid config");
  bdestroy(cfg->puredata.patch_directory);
  bdestroy(cfg->puredata.patch_file);
  bdestroy(cfg->chooser.pattern);

  bdestroy(cfg->broadcast.host);
  bdestroy(cfg->broadcast.source);
  bdestroy(cfg->broadcast.password);
  bdestroy(cfg->broadcast.mount);
  bdestroy(cfg->broadcast.name);
  bdestroy(cfg->broadcast.description);
  bdestroy(cfg->broadcast.genre);
  bdestroy(cfg->broadcast.url);
  free(cfg);
  return;
error:
  return;
}
