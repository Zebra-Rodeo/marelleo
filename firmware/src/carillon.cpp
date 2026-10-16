#include "carillon.h"
#include <driver/i2s_std.h>
#include <math.h>

namespace {
  constexpr uint32_t ECH = 44100;        // frequence d'echantillonnage
  constexpr size_t   BLOC = 256;         // trames ecrites d'un coup

  struct Note { float hertz; uint16_t ms; };

  // Une quinte montante, courte. Assez pour etre reconnue d'une piece a cote,
  // assez breve pour ne pas agacer celui qui rallume la boite dix fois.
  const Note BONJOUR[] = {{587.33f, 110}, {880.00f, 190}};

  // La meme a l'envers : on descend la quinte, et la note longue est en bas.
  // Une phrase qui finit en descendant se lit comme une fin, sans qu'on ait
  // rien a expliquer a personne.
  const Note ADIEU[] = {{880.00f, 110}, {587.33f, 190}};

  // Les bords d'une note doivent monter et descendre : un signal qui demarre a
  // pleine amplitude claque dans le haut-parleur.
  float enveloppe(uint32_t n, uint32_t total) {
    const uint32_t montee = ECH / 200;          // 5 ms
    const uint32_t chute  = ECH / 25;           // 40 ms
    if (n < montee)          return (float)n / montee;
    if (n + chute > total)   return (float)(total - n) / chute;
    return 1.0f;
  }
}

namespace {
void jouer(const Note* air, size_t notes,
           uint8_t bclk, uint8_t lrc, uint8_t dout, uint8_t volume, uint8_t volumeMax) {
  i2s_chan_handle_t tx = nullptr;
  i2s_chan_config_t canal = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  canal.dma_desc_num  = 4;
  canal.dma_frame_num = BLOC;
  if (i2s_new_channel(&canal, &tx, nullptr) != ESP_OK) return;

  i2s_std_config_t format = {
    .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(ECH),
    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
    .gpio_cfg = {
      .mclk = I2S_GPIO_UNUSED,
      .bclk = (gpio_num_t)bclk,
      .ws   = (gpio_num_t)lrc,
      .dout = (gpio_num_t)dout,
      .din  = I2S_GPIO_UNUSED,
      .invert_flags = {false, false, false},
    },
  };
  if (i2s_channel_init_std_mode(tx, &format) != ESP_OK) { i2s_del_channel(tx); return; }
  if (i2s_channel_enable(tx) != ESP_OK)                 { i2s_del_channel(tx); return; }

  // Le carillon suit la molette : une boite rallumee a trois heures du matin
  // ne doit pas reveiller la maison. Le plancher evite qu'il devienne inaudible
  // et donc inutile.
  float ampleur = volumeMax ? (float)volume / volumeMax : 1.0f;
  if (ampleur < 0.25f) ampleur = 0.25f;
  const float crete = 9000.0f * ampleur;        // sur 32767

  static int16_t tampon[BLOC * 2];
  for (size_t k = 0; k < notes; k++) {
    const Note& note = air[k];
    const uint32_t total = (uint32_t)ECH * note.ms / 1000;
    const float pas = 2.0f * (float)M_PI * note.hertz / ECH;
    uint32_t n = 0;
    while (n < total) {
      size_t trames = min((uint32_t)BLOC, total - n);
      for (size_t i = 0; i < trames; i++, n++) {
        int16_t v = (int16_t)(sinf(pas * n) * enveloppe(n, total) * crete);
        tampon[i * 2] = tampon[i * 2 + 1] = v;
      }
      size_t ecrits = 0;
      i2s_channel_write(tx, tampon, trames * 2 * sizeof(int16_t), &ecrits, 200);
    }
  }

  // Un silence final, le temps que le DMA vide ce qui reste : sans lui, couper
  // le canal tronque la derniere note et laisse un claquement.
  memset(tampon, 0, sizeof tampon);
  for (int i = 0; i < 8; i++) {
    size_t ecrits = 0;
    i2s_channel_write(tx, tampon, sizeof tampon, &ecrits, 200);
  }

  i2s_channel_disable(tx);
  i2s_del_channel(tx);
}
}  // namespace

void carillon(uint8_t bclk, uint8_t lrc, uint8_t dout, uint8_t volume, uint8_t volumeMax) {
  jouer(BONJOUR, sizeof BONJOUR / sizeof *BONJOUR, bclk, lrc, dout, volume, volumeMax);
}

void carillonAdieu(uint8_t bclk, uint8_t lrc, uint8_t dout, uint8_t volume, uint8_t volumeMax) {
  jouer(ADIEU, sizeof ADIEU / sizeof *ADIEU, bclk, lrc, dout, volume, volumeMax);
}
