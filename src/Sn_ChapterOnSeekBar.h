#ifndef INCLUDE_Sn_ChapterOnSeekBar_H
#define INCLUDE_Sn_ChapterOnSeekBar_H
#pragma once
#include <algorithm>

namespace Snow
{
	/*���̌`*/
	enum TriType {
		none = 0,
		L = 1,/*����  �X�L�b�v��*/
		LR = 2,/*  ��  �`���v�^�[*/
		R = 3,/*�E��  �X�L�b�v��*/
	};


	class util {
	public:
		/* time msec  -->  1:02:03.4 */
		static std::wstring TimeToString(int totalMSec, bool symbol = false, bool decimal = false)
		{
			bool plus = 0 <= totalMSec ? true : false;
			totalMSec = std::abs(totalMSec);
			int totalSec = totalMSec / 1000;
			int	h = totalSec / 60 / 60;
			int m = (totalSec - h * 60 * 60) / 60;
			int s = totalSec - h * 60 * 60 - m * 60;
			/*0.1sec�P��*/
			int tenth = totalMSec - h * 60 * 60 * 1000 - m * 60 * 1000 - s * 1000;
			tenth /= 100;

			TCHAR h_str[32], ms_str[32], tenth_str[32];
			::wsprintf(h_str, TEXT("%d:"), h);
			::wsprintf(ms_str, TEXT("%02d:%02d"), m, s);
			::wsprintf(tenth_str, TEXT(".%1d"), tenth);
			std::wstring text;
			text += symbol && plus ? L"+" : std::wstring();
			text += !plus ? L"-" : std::wstring();
			//text += 0 < h ? std::wstring(h_str) : std::wstring();
			text += std::wstring(h_str);
			text += std::wstring(ms_str);
			text += decimal ? std::wstring(tenth_str) : std::wstring();
			return text;
		}
	};


	/*
	�V�[�N�o�[�̑I�𒆃`���v�^�[
		Select Chapter, Select Range, Slide Chapter
	*/
	class CChapterOnSeekBar
	{
	public:
		/*�`���v�^�[��TriType�I��*/
		/*����  ��  �E��*/
		TriType GetTriType(const std::map<int, CChapterMap::CHAPTER>::const_iterator ch);

		/*���I��*/
	private:
		int selChp = -1;
	public:
		bool HasSelect() { return  0 <= selChp; }
		bool IsSelect(int chp) { return chp == selChp; }
		int GetSelect() { return selChp; }
		void ClearSelect() { selChp = -1; }
		/*���@�I�𒆁H*/
		void UpdateSelectChp(const std::map<int, CChapterMap::CHAPTER> &chMap,
			double mouTimeRatio, int duration, double triWRatio, bool margin);
		/*mouse on ���H*/
		bool IsOnTri(double mouTimeRatio, double triTimeRatio,
			double triWRatio, TriType type, bool hasMargin);

		/*�X�L�b�v���͈�*/
	private:
		int selRangeL = -1, selRangeR = -1;
	public:
		bool HasSelectRange() { return selRangeL != -1 && selRangeR != -1; }
		bool IsSelectRange(int chp) { return HasSelectRange() && (chp == selRangeL || chp == selRangeR); }
		bool IsSelectRange(int chpL, int chpR) { return HasSelectRange() && chpL == selRangeL && chpR == selRangeR; }
		std::vector<int> GetSelectRange() { return std::vector<int>{ selRangeL, selRangeR }; }
		void ClearSelectRange() { selRangeL = selRangeR = -1; }
		/*���͈́@���W*/
		std::vector<std::pair<int, int>> CollectSkipRange(const std::map<int, CChapterMap::CHAPTER> &chMap);
		/*�X�L�b�v�����H*/
		bool IsSkipChapter(const std::map<int, CChapterMap::CHAPTER> &chMap, int time);
		/*���͈́@�I�𒆁H*/
		void UpdateSelectRange(const std::map<int, CChapterMap::CHAPTER> &chMap, double mouTimeRatio, int duration);
		/*�X�L�b�v���͈͂��쐬*/
		/*time����V�������͈͂��쐬�ł��邩*/
		int MakeSkipRange(const std::map<int, CChapterMap::CHAPTER> &chMap, int time);
		std::vector<int> MakeSkipRange2(const std::map<int, CChapterMap::CHAPTER> &chMap,
			int begin, int end);
		std::vector<std::pair<bool, int>> MakeSkipRange3(const std::map<int, CChapterMap::CHAPTER> &chMap,
			int begin, int end);
		/*�������͈́@�I��*/
		void UpdateSelect(const std::map<int, CChapterMap::CHAPTER> &chMap, double mouTimeRatio, int duration, double triWRatio);

		/*�X���C�h��*/
	private:
		int slideChp = -1;
		int slideChpFrom = -1;
		TriType slideChpType;
	public:
		bool HasSlide() { return  0 <= slideChpFrom; }
		void StartSlide(int from, TriType type) { slideChpFrom = from; slideChpType = type; }
		void ClearSlide() { slideChpFrom = -1; }
		int GetSlide() { return slideChp; }
		int GetBeforeSlide() { return slideChpFrom; }
		TriType GetSlideChpType() { return slideChpType; }
		/*�X���C�h���̈ʒu���X�V*/
		void UpdateSlide(const std::map<int, CChapterMap::CHAPTER> &chMap,
			double mouTimeRatio, int duration, double triWRatio);
		/*�X���C�h�m��*/
		void ConfirmSlide(CChapterMap &Chapter,
			const std::map<int, CChapterMap::CHAPTER> &chMap);

		/*TimePanel�ɕ\������e�L�X�g�擾*/
		std::wstring GetTimePanelText(int playTime, int mouseTime, int duration);
	};


	/*�`���v�^�[��TriType�I��*/
	/*���� or �� or �E��*/
	TriType CChapterOnSeekBar::GetTriType(const std::map<int, CChapterMap::CHAPTER>::const_iterator ch)
	{
		TriType type = ch->second.IsSkipBegin() ? TriType::L
			: ch->second.IsSkipEnd() ? TriType::R
			: TriType::LR;
		return type;
	}


	/*���I��*/
	/*���@�I�𒆁H*/
	void CChapterOnSeekBar::UpdateSelectChp(const std::map<int, CChapterMap::CHAPTER> &chMap,
		double mouTimeRatio, int duration, double triWRatio, bool margin)
	{
		for (auto ch = chMap.begin(); ch != chMap.end(); ++ch) {
			double chTimeRatio = 1.0 * ch->first / duration;
			chTimeRatio = std::clamp(chTimeRatio, 0.0, 1.0);
			TriType type = GetTriType(ch);
			bool isOn = IsOnTri(mouTimeRatio, chTimeRatio, triWRatio, type, margin);
			if (isOn) {
				selChp = ch->first;
				return;
			}
		}
	}
	/*mouse on ���H*/
	bool CChapterOnSeekBar::IsOnTri(double mouTimeRatio, double triTimeRatio,
		double triWRatio, TriType type, bool hasMargin)
	{
		double mou = mouTimeRatio;
		double triC = triTimeRatio;
		double triL = triC - triWRatio / 2;
		double triR = triC + triWRatio / 2;
		double m = hasMargin ? triWRatio : 0;

		bool isOn = false;
		if (type == TriType::L)
			isOn = triL - m <= mou && mou <= triC + m;
		else if (type == TriType::LR)
			isOn = triL - m <= mou && mou <= triR + m;
		else if (type == TriType::R)
			isOn = triC - m <= mou && mou <= triR + m;
		return isOn;
	}

	/*���͈́@���W*/
	std::vector<std::pair<int, int>> CChapterOnSeekBar::CollectSkipRange(const std::map<int, CChapterMap::CHAPTER> &chMap)
	{
		std::vector<std::pair<int, int>> range;
		bool findBegin = false;
		int begin = 0;
		for (auto ch = chMap.begin(); ch != chMap.end(); ++ch) {
			if (findBegin == false) {
				if (ch->second.IsSkipBegin()) {
					findBegin = true;
					begin = ch->first;
				}
			}
			else {
				if (ch->second.IsSkipEnd()) {
					int end = ch->first;
					range.emplace_back(begin, end);
					findBegin = false;
				}
			}
		}
		return range;
	}

	/*�X�L�b�v�����H*/
	bool CChapterOnSeekBar::IsSkipChapter(const std::map<int, CChapterMap::CHAPTER> &chMap,
		int time)
	{
		const auto range = CollectSkipRange(chMap);
		for (const auto &r : range)
		{
			if (r.first == time || r.second == time)
				return true;
		}
		return false;
	}


	/*���͈�*/
	/*���͈́@�I�𒆁H*/
	void CChapterOnSeekBar::UpdateSelectRange(const std::map<int, CChapterMap::CHAPTER> &chMap,
		double mouTimeRatio, int duration)
	{
		const auto range = CollectSkipRange(chMap);
		for (const auto &r : range)
		{
			int begin = r.first;
			int end = r.second;
			double mou = mouTimeRatio;
			double chpR = 1.0 * begin / duration;
			double chpL = 1.0 * end / duration;
			chpR = std::clamp(chpR, 0.0, 1.0);
			chpL = std::clamp(chpL, 0.0, 1.0);
			bool isIn = chpR <= mou && mou <= chpL;
			if (isIn) {
				selRangeL = begin;
				selRangeR = end;
				return;
			}
		}
	}

	/*�X�L�b�v���͈͂��쐬*/
	/*�X�L�b�v��time����͈͂��쐬�ł��邩*/
	/* success  -->  return �X�L�b�v��*/
	/* failure  -->         -1*/
	int CChapterOnSeekBar::MakeSkipRange(const std::map<int, CChapterMap::CHAPTER> &chMap,
		int time)
	{
		const int SkipLen = 3 * 1000;
		const int Margin = 1 * 1000;
		auto chpR = chMap.lower_bound(time);
		auto chpL = std::prev(chpR, 1);
		int timeR = chpR != chMap.end() ? chpR->first : CChapterMap::CHAPTER_POS_MAX;
		int timeL = chpL != chMap.end() ? chpL->first : 0;
		bool hasBlank = timeL < time - Margin && time + SkipLen + Margin < timeR;
		if (hasBlank) {
			int skipR = time + SkipLen;
			skipR = std::clamp(skipR, skipR, CChapterMap::CHAPTER_POS_MAX);
			return skipR;
		}
		else
			return -1;
	}
	std::vector<int> CChapterOnSeekBar::MakeSkipRange2(const std::map<int, CChapterMap::CHAPTER> &chMap,
		int begin, int end)
	{
		const std::vector<int> Len{ 3, 6, 9, 12, 20, 22 };
		const int Margin = 1;

		auto ch = chMap.find(end);
		if (ch == chMap.end())
			return std::vector<int>();
		auto next = std::next(ch, 1);
		int limit = next != chMap.end() ? next->first : CChapterMap::CHAPTER_POS_MAX;

		std::vector<int> canSkip;
		for (int len : Len) {
			bool hasBlank = begin + len * 1000 + Margin * 1000 < limit;
			if (hasBlank) {
				int newEnd = begin + len * 1000;
				newEnd = std::clamp(newEnd, newEnd, CChapterMap::CHAPTER_POS_MAX);
				canSkip.emplace_back(len);
			}
		}
		return canSkip;
	}
	std::vector<std::pair<bool, int>> CChapterOnSeekBar::MakeSkipRange3(const std::map<int, CChapterMap::CHAPTER> &chMap,
		int begin, int end)
	{
		const std::vector<int> Len{ 3, 6, 9, 12, 20, 22 };
		const int Margin = 1;

		auto ch = chMap.find(end);
		if (ch == chMap.end())
			return std::vector<std::pair<bool, int>>();
		auto next = std::next(ch, 1);
		int limit = next != chMap.end() ? next->first : CChapterMap::CHAPTER_POS_MAX;

		std::vector<std::pair<bool, int>>  canSkip;
		for (int len : Len) {
			bool hasBlank = begin + len * 1000 + Margin * 1000 < limit;
			if (hasBlank) {
				int newEnd = begin + len * 1000;
				newEnd = std::clamp(newEnd, newEnd, CChapterMap::CHAPTER_POS_MAX);
				canSkip.emplace_back(std::pair<bool, int>(true, len));
			}
			else
				canSkip.emplace_back(std::pair<bool, int>(false, len));
		}
		return canSkip;
	}

	/*�������͈́@�I��*/
	void CChapterOnSeekBar::UpdateSelect(const std::map<int, CChapterMap::CHAPTER> &chMap,
		double mouTimeRatio, int duration, double triWRatio)
	{
		/*�����ד��m���ƑI�����ɂ����̂Ń}�[�W���L���̗����Ń`�F�b�N����*/
		bool margin = false;
		UpdateSelectChp(chMap, mouTimeRatio, duration, triWRatio, margin);
		if (HasSelect() == false)
			UpdateSelectRange(chMap, mouTimeRatio, duration);
		/*�}�[�W���L��Ń`�F�b�N*/
		if (HasSelect() == false && HasSelectRange() == false) {
			margin = true;
			UpdateSelectChp(chMap, mouTimeRatio, duration, triWRatio, margin);
		}
	}


	/*�X���C�h��*/
	/*�X���C�h���̈ʒu���X�V*/
	void CChapterOnSeekBar::UpdateSlide(const std::map<int, CChapterMap::CHAPTER> &chMap,
		double mouTimeRatio, int duration, double triWRatio)
	{
		/*�X���C�h�����E�̃`���v�^�[�̎�O�܂łɐ���*/
		auto chp = chMap.find(slideChpFrom);
		if (chp == chMap.end())
			return;
		auto prev = std::prev(chp, 1);
		auto next = std::next(chp, 1);
		int timeL = prev != chMap.end() ? prev->first : 0;
		int timeR = next != chMap.end() ? next->first : CChapterMap::CHAPTER_POS_MAX;
		double ratioL = 1.0 * timeL / duration;
		double ratioR = 1.0 * timeR / duration;
		ratioL = 0 < ratioL ? ratioL + triWRatio : 0;
		ratioR = ratioR < 1.0 ? ratioR - triWRatio : 1.0;

		double pos = mouTimeRatio;
		pos = std::clamp(pos, ratioL, ratioR);
		slideChp = static_cast<int>(pos * duration);
	}

	/*�X���C�h�m��*/
	void CChapterOnSeekBar::ConfirmSlide(CChapterMap &Chapter,
		const std::map<int, CChapterMap::CHAPTER> &chMap)
	{
		int from = slideChpFrom;
		int to = GetSlide();
		std::map<int, CChapterMap::CHAPTER>::const_iterator	it;
		it = chMap.find(from);
		if (it == chMap.end())
			return;
		std::pair<int, CChapterMap::CHAPTER> ch = *it;
		ch.first = to;
		Chapter.Insert(ch, from);
		slideChpFrom = -1;
	}


	/*TimePanel�ɕ\������e�L�X�g�擾*/
	std::wstring CChapterOnSeekBar::GetTimePanelText(int playTime, int mouseTime, int duration)
	{
		std::wstring text;
		if (HasSlide())
		{
			int time = GetSlide();
			int from = GetBeforeSlide();
			int diff = time - from;
			std::wstring timeS = util::TimeToString(time);
			std::wstring diffS = util::TimeToString(diff, true);
			text = timeS + L"  (" + diffS + L")";
		}
		else if (HasSelect())
		{
			int time = GetSelect();
			time = time == CChapterMap::CHAPTER_POS_MAX ? duration : time;
			int diff = playTime - time;
			std::wstring timeS = util::TimeToString(time);
			std::wstring diffS = util::TimeToString(diff, true);
			text = timeS + L" " + diffS;
		}
		else
		{
			int time = mouseTime;
			time = time == CChapterMap::CHAPTER_POS_MAX ? duration : time;
			std::wstring timeS = util::TimeToString(time);
			text = timeS;
		}
		return L"  " + text + L"  ";
	}






}
#endif // INCLUDE_Sn_ChapterOnSeekBar_H