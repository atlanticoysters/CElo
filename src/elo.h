/*
Copyright (C) 2018 CElo.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <math.h>
#include <stdlib.h>

#define ELO_RETURN_ON_ERROR(X) if (X != ELO_SUCCESS) { return X; }

typedef struct {
	double rating;
	double default_K;
} elo_player;

typedef enum {
	ELO_SUCCESS, ELO_INVALID_SCORE, ELO_NEGATIVE_K, ELO_NONPOSITIVE_RATING,
	ELO_ZERO_LENGTH
} elo_error;

double elo_expected(double player1_rating, double player2_rating) {
	return 1 / (1 + pow(10, (player2_rating - player1_rating) / 400));
}

elo_error elo_update_1v1_custom_K(elo_player *player1, elo_player *player2,
	double score_player1, double K_player1, double K_player2) {

	if (!(score_player1 == 0 || score_player1 == 0.5 || score_player1 == 1)) {
		return ELO_INVALID_SCORE;
	}

	if (K_player1 <= 0 || K_player2 <= 0) {
		return ELO_NEGATIVE_K;
	}

	double expected_player1 = elo_expected(player1->rating, player2->rating);
	double expected_player2 = 1 - expected_player1;
	double score_player2 = 1 - score_player1;

	player1->rating += K_player1 * (score_player1 - expected_player1);
	player2->rating += K_player2 * (score_player2 - expected_player2);
	return ELO_SUCCESS;
}

elo_error elo_update_1v1(elo_player *player1, elo_player *player2,
	double score_player1) {

	return elo_update_1v1_custom_K(player1, player2, score_player1,
		player1->default_K, player2->default_K);
}

double elo_convert_to_bradley_terry(double elo_rating) {
	return pow(10, elo_rating / 400);
}

elo_error elo_minimum_rating(elo_player *team, size_t team_len, double *ret) {
	if (team_len == 0) {
		return ELO_ZERO_LENGTH;
	}

	double minimum_rating = team[0].rating;
	size_t i;
	for (i = 1; i < team_len; i++) {
		if (team[i].rating < minimum_rating) {
			minimum_rating = team[i].rating;
		}
	}

	*ret = minimum_rating;
	return ELO_SUCCESS;
}

double elo_make_positive(double rating, double minimum_rating) {
	return rating - minimum_rating + 1;
}

elo_error elo_sum_rating(elo_player *team, size_t team_len, double *ret) {
	if (team_len == 0) {
		return ELO_ZERO_LENGTH;
	}
	double sum = 0;
	size_t i;
	for (i = 0; i < team_len; i++) {
		if (team[i].rating <= 0) {
			return ELO_NONPOSITIVE_RATING;
		}
		sum += elo_convert_to_bradley_terry(team[i].rating);
	}

	*ret = sum;
	return ELO_SUCCESS;
}

elo_error elo_update_team_v_team(elo_player *team1, elo_player *team2,
	size_t team1_len, size_t team2_len, double score_team1, double K_team1,
double K_team2) {

	if (team1_len == 0 || team2_len == 0) {
		return ELO_ZERO_LENGTH;
	}

	if (K_team1 <= 0 || K_team2 <= 0) {
		return ELO_NEGATIVE_K;
	}

	double rating_team1;
	elo_error rating_team1_err = elo_sum_rating(team1, team1_len,
		&rating_team1);
	ELO_RETURN_ON_ERROR(rating_team1_err);

	double rating_team2;
	elo_error rating_team2_err = elo_sum_rating(team2, team2_len,
		&rating_team2);
	ELO_RETURN_ON_ERROR(rating_team2_err);

	double expected_team1 = rating_team1 / (rating_team1 + rating_team2);
	double expected_team2 = 1 - expected_team1;
	double score_team2 = 1 - score_team1;

	double delta_team1 = K_team1 * (score_team1 - expected_team1);
	double delta_team2 = K_team2 * (score_team2 - expected_team2);

	size_t i;
	for (i = 0; i < team1_len; i++) {
		team1[i].rating += (elo_convert_to_bradley_terry(team1[i].rating) /
			rating_team1) * delta_team1;
	}

	for (i = 0; i < team2_len; i++) {
		team2[i].rating += (elo_convert_to_bradley_terry(team2[i].rating) /
			rating_team2) * delta_team2;
	}
	return ELO_SUCCESS;
}
