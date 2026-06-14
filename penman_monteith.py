def buharlasma_simulasyonu_penman_monteith():
    print("--- Akıllı Yüzer GES: Penman-Monteith Enerji ve Su Simülasyonu ---")

    # --- KAYNAK TABANLI PARAMETRELER ---
    # Korkmaz (2015) saha deneyine göre buharlaşma azalma oranı %59.2 [3]
    AZALMA_ORANI = 0.592
    # Psikrometrik sabit (kPa/C) [2]
    GAMMA = 0.067

    try:
        # Girdiler (Hata önlemek için virgül/nokta düzeltmesi yapılmıştır)
        baraj_alani = float(input("Baraj yüzey alanı (m2): ").replace(',', '.'))
        modul_sayisi = int(input("Kurulacak 100x100cm modül sayısı: "))

        print("\n--- Yerel Meteorolojik Verileri Giriniz ---")
        T = float(input("Ortalama Hava Sıcaklığı (°C): ").replace(',', '.'))
        RH = float(input("Ortalama Bağıl Nem (%): ").replace(',', '.'))
        u2 = float(input("2m yükseklikteki Rüzgâr Hızı (m/s): ").replace(',', '.'))
        Rs = float(input("Günlük Güneş Radyasyonu (MJ/m2/gün) [Örn: 15-25]: ").replace(',', '.'))

        # --- PENMAN-MONTEITH ALGORİTMASI (FAO-56) [1, 2] ---
        # 1. Buhar Basıncı Eğrisinin Eğimi (Delta)
        es = 0.6108 * (2.71828 ** (17.27 * T / (T + 237.3))) # Doygun buhar basıncı
        delta = (4098 * es) / ((T + 237.3) ** 2)

        # 2. Gerçek Buhar Basıncı (ed)
        ed = es * (RH / 100)

        # 3. Net Radyasyon Tahmini (Rn) [2]
        # Basitleştirilmiş: Rs'nin bir kısmı yansır (Albedo ~0.23)
        Rn = 0.77 * Rs

        # 4. Günlük Referans Evapotranspirasyon (Eto - mm/gün) [1]
        # G (Toprak ısı akısı) günlük hesaplamalarda 0 kabul edilir [2].
        pay = (0.408 * delta * Rn) + (GAMMA * (900 / (T + 273)) * u2 * (es - ed))
        payda = delta + (GAMMA * (1 + 0.34 * u2))
        eto_dogal = pay / payda

        # --- TASARRUF VE ENERJİ HESAPLARI ---
        toplam_ges_alani = modul_sayisi * 1.0 # 100x100cm = 1m2 [4]
        kaplanma_yuzdesi = (toplam_ges_alani / baraj_alani) * 100

        if kaplanma_yuzdesi > 100:
            print("\n!!! HATA: GES alanı barajdan büyük olamaz.")
            return

        # Toplam günlük kayıp (m3) = Alan * Derinlik(mm) / 1000
        gunluk_dogal_kayip_m3 = (baraj_alani * eto_dogal) / 1000

        # Tasarruf: Sadece kapatılan alanda %59.2 verimlilik [3]
        gunluk_tasarruf_m3 = (toplam_ges_alani * eto_dogal / 1000) * AZALMA_ORANI

        # Enerji Üretimi: 0.57 m2 -> 20.67W ise 1 m2 -> 36.26W ortalama [5]
        gunluk_enerji_kwh = (toplam_ges_alani * 36.26 * 24) / 1000

        # --- TERMİNAL ÇIKTILARI ---
        print("\n" + "="*45)
        print(f"PENMAN-MONTEITH ANALİZ SONUÇLARI")
        print("="*45)
        print(f"Doğal Buharlaşma Hızı: {eto_dogal:.2f} mm/gün")
        print(f"Barajın Kaplanma Oranı: %{kaplanma_yuzdesi:.4f}")
        print(f"Günlük Su Tasarrufu: {gunluk_tasarruf_m3:.2f} m3/gün")
        print(f"Günlük Enerji Üretimi: {gunluk_enerji_kwh:.2f} kWh/gün")
        print("-" * 45)
        print(f"Yıllık Projeksiyon Su Tasarrufu: {gunluk_tasarruf_m3 * 365:,.2f} m3/yıl")

        # Kaynak [6]'ya göre biyolojik uyarı:
        if kaplanma_yuzdesi > 10:
            print("\n[DİKKAT] E.Coli bakteri artış riski! Otonom kontrol devreye girmeli [6].")

    except ValueError:
        print("Lütfen geçerli bir sayısal değer girin.")

if __name__ == "__main__":
    buharlasma_simulasyonu_penman_monteith()

