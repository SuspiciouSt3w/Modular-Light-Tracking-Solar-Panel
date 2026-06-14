# Modüler Ayçiçeği Yüzer Güneş Enerjisi Santrali
**FL DigiField**  Samsung Solve for Tomorrow 2026 

Baraj yüzeylerindeki buharlaşma kayıplarını azaltmak için tasarlanan sürdürülebilir ve modüler
bir GES' için ortalama Kesikköprü Barajı verileri ile simülasyon sonuçları, simülasyon kodları ve Wokwi uygulamasında ESP32 kodları

---

## Proje Çıktıları (Kesikköprü Barajı, %1 kaplama oranı)
- Buharlaşma azaltımı: **%59,2**
- Günlük su tasarrufu: **134,76 m³/gün**
- Yıllık su tasarrufu: **49.186 m³/yıl**
- Günlük enerji üretimi: **56.565,60 kWh/gün**
- Yıllık CO₂ önlemi: **~10.300 ton**

---

## Donanım
| Bileşen | Görev |
|---|---|
| ESP32 WROOM DevKit C V4 | Ana mikrodenetleyici, WiFi → Firebase |
| BME280 | Sıcaklık, nem, atmosferik basınç |
| DS18B20 (su geçirmez) | Su yüzeyi sıcaklığı |
| 4× LDR | Güneş takip dizilimi (kuadrant) |
| INA219 | MPPT verim ölçümü |
| 28BYJ-48 + ULN2003 | Güneş takip step motoru |
| CN3791 MPPT | Şarj yönetimi |

---

## Dosyalar
| Dosya | Açıklama |
|---|---|
| `penman_monteith.py` | Proje ölçeği su/enerji simülasyonu (FAO-56) |
| `solartrackingpanel_simulation.ino` | ESP32 ana kodu güneş takibi ve simülasyon raporu |
| `diagram.json` | Wokwi şema düzeni |
| `libraries.txt` | Wokwi için kullanılan sensör ve kütüphaneler  |

---

## Simülasyon
Wokwi elektronik simülasyon linki: [Wokwi Simülasyonu](https://wokwi.com/projects/463439764093167617)

---

## Kaynaklar
- Şenli, H. (2023). Yüzen Güneş Enerjisi Sistemleri. *JEPS*, 35(4), 418–427.
- Bayramoğlu, E. (2013). Penman-Monteith Yöntemi. *Kastamonu Üniversitesi Orman Fakültesi Dergisi*, 13(2).
- Korkmaz, M. (2015). Buharlaşma azaltım oranı (%59,2) saha deneyi.
- DSİ (2024). Yüzer GES'lerle hem temiz enerji hem su tasarrufu.
